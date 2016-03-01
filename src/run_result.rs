use std::collections::BTreeMap;
use utils;
use error;
use rustc_serialize::json::{self, ToJson, Json};
use rustc_serialize::base64::{self, ToBase64};
use num::traits::FromPrimitive;
use num;
use std::io::Cursor;
use byteorder::{ReadBytesExt, WriteBytesExt, BigEndian, LittleEndian};
use std::fs;
use error::PoeError;
use std::io::{Read, Write, Seek, SeekFrom};
use config;
use std::collections::HashSet;
use std::iter::FromIterator;
use snippet::Snippet;
use compiler::Compiler;
use std::process::Output;

#[derive(RustcDecodable, RustcEncodable)]
struct RunResultMetadata {
    pub exit: i32,
    pub result: i32,
    pub message: String,
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
            let (out, truncated) = read_output(&snip, &comp);
            map.insert("output".to_string(), out.to_json());
            map.insert("truncated".to_string(), truncated.to_json());
        },
        Err(_) => {
            map.insert("result".to_string(), None::<i32>.to_json());
        }
    }

    Json::Object(map)
}

fn read_output(snip: &Snippet, comp: &Compiler) -> (Vec<(u8, String)>, bool) {
    let f = |out: &mut Vec<(u8, String)>| {
        let mut out_file = stry!(fs::File::open(format!("{}/results/{}.output", &snip.basedir(), &comp.id)));
        while let Ok(fd) = out_file.read_u8() {
            let len = stry!(out_file.read_u32::<LittleEndian>());
            if len > 1024 * 1024 { return Err("broken input?".to_string()); } // TODO
            let mut body = vec![0; len as usize];
            stry!(out_file.read_exact(&mut body));
            out.push((fd, String::from_utf8_lossy(&body).into_owned()));
        }
        Ok(())
    };

    let mut out = vec![];
    let truncated = Ok(()) != f(&mut out);
    (out, truncated)
}

pub fn parse_and_save(snip: &Snippet, comp: &Compiler, output: Output) -> Result<(), PoeError> {
    if output.status.success() {
        if output.stderr.len() < 8 {
            return Err(PoeError::from("failed sandbox (result)"));
        }

        let (metavec, msgvec) = output.stderr.split_at(8);
        let mut rdr = Cursor::new(metavec);
        let reason = rdr.read_i32::<LittleEndian>().unwrap();
        let exit = rdr.read_i32::<LittleEndian>().unwrap();
        let msg_str = String::from_utf8_lossy(&msgvec);
        let meta = RunResultMetadata { exit: exit, result: reason, message: (&*msg_str).to_string() };

        let mut meta_file = try!(fs::File::create(format!("{}/results/{}.json", &snip.basedir(), &comp.id)));
        try!(meta_file.write(json::encode(&meta).unwrap().as_bytes()));

        let mut out_file = try!(fs::File::create(format!("{}/results/{}.output", &snip.basedir(), &comp.id)));
        try!(out_file.write(&output.stdout));

        Ok(())
    } else {
        println!("stdout: {}", String::from_utf8_lossy(&output.stdout).into_owned());
        println!("stderr: {}", String::from_utf8_lossy(&output.stderr).into_owned());
        Err(PoeError::from("failed sandbox (error)"))
    }
}