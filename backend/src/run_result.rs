use std::collections::BTreeMap;
use rustc_serialize::json::{self, ToJson, Json};
use std::io::Cursor;
use byteorder::{ReadBytesExt, LittleEndian};
use std::fs;
use error::PoeError;
use std::io::{Read, Write};
use snippet::Snippet;
use compiler::Compiler;
use std::process::Output;

#[derive(RustcDecodable, RustcEncodable)]
struct RunResultMetadata {
    pub exit: i32,
    pub result: i32,
    pub elapsed_ms: i32,
    pub message: String,
    pub truncated: bool,
}

pub fn open_render(snip: &Snippet, comp: &Compiler) -> Json {
    let mut map = BTreeMap::new();
    map.insert("compiler".to_string(), comp.render());

    match fs::File::open(format!("{}/results/{}.json", &snip.basedir(), &comp.id)) {
        Ok(mut file) => {
            let mut encoded = String::new();
            file.read_to_string(&mut encoded).unwrap();
            let meta: RunResultMetadata = json::decode(&encoded).unwrap();
            map.insert("exit".to_string(), meta.exit.to_json());
            map.insert("result".to_string(), meta.result.to_json());
            map.insert("elapsed_ms".to_string(), meta.elapsed_ms.to_json());
            map.insert("message".to_string(), meta.message.to_json());
            map.insert("truncated".to_string(), meta.truncated.to_json());
            map.insert("output".to_string(), read_output_str(&snip, &comp).to_json());
        },
        Err(_) => {
            map.insert("result".to_string(), None::<i32>.to_json());
        }
    }

    Json::Object(map)
}

pub fn read_stdout_raw(snip: &Snippet, comp: &Compiler) -> Vec<u8> {
    let mut out = vec![];
    for (fd, raw) in read_output_raw(&snip, &comp) {
        if fd == 1 {
            out.extend_from_slice(&raw); // TODO: this copies
        }
    }
    out
}

fn read_output_str(snip: &Snippet, comp: &Compiler) -> Vec<(u32, String)> {
    read_output_raw(&snip, &comp).iter().map(|pair| {
        (pair.0, String::from_utf8_lossy(&pair.1).into_owned())
    }).collect()
}

fn read_output_raw(snip: &Snippet, comp: &Compiler) -> Vec<(u32, Vec<u8>)> {
    let f = |out: &mut Vec<(u32, Vec<u8>)>| {
        let mut out_file = stry!(fs::File::open(format!("{}/results/{}.output", &snip.basedir(), &comp.id)));
        while let Ok(fd) = out_file.read_u32::<LittleEndian>() {
            let len = stry!(out_file.read_u32::<LittleEndian>());
            if len > 1024 * 1024 { return Err("broken input?".to_string()); } // TODO
            let mut body = vec![0; len as usize];
            stry!(out_file.read_exact(&mut body));
            out.push((fd, body));
        }
        Ok(())
    };

    let mut out = vec![];
    if let Err(err) = f(&mut out) {
        println!("{}", err);
    }
    out
}

pub fn parse_and_save(snip: &Snippet, comp: &Compiler, output: Output) -> Result<(), PoeError> {
    let output_limit = 65536;

    if output.status.success() {
        if output.stderr.len() < 12 {
            return Err(PoeError::from("failed sandbox (result)"));
        }

        let (metavec, msgvec) = output.stderr.split_at(12);
        let mut rdr = Cursor::new(metavec);
        let reason = rdr.read_i32::<LittleEndian>().unwrap();
        let exit = rdr.read_i32::<LittleEndian>().unwrap();
        let elapsed_ms = rdr.read_i32::<LittleEndian>().unwrap();
        let msg_str = String::from_utf8_lossy(&msgvec);
        let trunc = output.stdout.len() > output_limit;
        let meta = RunResultMetadata { exit: exit, result: reason, message: msg_str.into_owned(), truncated: trunc, elapsed_ms: elapsed_ms };

        let mut meta_file = fs::File::create(format!("{}/results/{}.json", &snip.basedir(), &comp.id))?;
        meta_file.write(json::encode(&meta).unwrap().as_bytes())?;

        let mut out_file = fs::File::create(format!("{}/results/{}.output", &snip.basedir(), &comp.id))?;
        out_file.write(if trunc { &output.stdout[..output_limit] } else { &output.stdout })?;

        Ok(())
    } else {
        println!("stdout: {}", String::from_utf8_lossy(&output.stdout).into_owned());
        println!("stderr: {}", String::from_utf8_lossy(&output.stderr).into_owned());
        Err(PoeError::from("failed sandbox (error)"))
    }
}
