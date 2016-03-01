use rustc_serialize::json::{self, Json};
use std::iter::FromIterator;
use std::fs;
use std::io::Read;
use std::env;
use std::collections::BTreeMap;
use compiler::Compiler;

struct Config {
    datadir: String,
    runner: String,
    compilers: BTreeMap<String, BTreeMap<String, Compiler>>,
}

pub fn compiler(lang: &str, id: &str) -> Option<&'static Compiler> {
    get().compilers.get(lang).and_then(|cmap| cmap.get(id))
}

pub fn compilers(lang: &str) -> Option<&BTreeMap<String, Compiler>> {
    get().compilers.get(lang)
}

pub fn datadir() -> String {
    get().datadir.clone()
}

pub fn basedir() -> String {
    format!("{}/env/base", get().datadir)
}

pub fn runner() -> String {
    get().runner.clone()
}

static mut DATA: *const Config = 0 as *const Config;
pub fn load() {
    if unsafe { DATA != (0 as *const Config) } {
        panic!("config already initialized");
    }

    let conf_file = env::args().nth(1).unwrap();
    let mut file = fs::File::open(conf_file).unwrap();
    let mut encoded = String::new();
    file.read_to_string(&mut encoded).unwrap();
    let data = Json::from_str(&encoded).unwrap();

    let datadir = data.find("datadir").unwrap().as_string().unwrap().to_string();
    let runner = data.find("runner").unwrap().as_string().unwrap().to_string();
    let compilers = BTreeMap::from_iter(
        data.find("compilers").unwrap().as_object().unwrap().iter().map(|ls| {
            let (lang, smap) = ls;
            let nsmap = BTreeMap::from_iter(
                smap.as_object().unwrap().iter().map(|vc| {
                    let (pcid, conf) = vc;
                    (pcid.to_string(), Compiler {
                        id: pcid.to_string(),
                        lang: lang.clone(),
                        version: conf.find("version").unwrap().as_string().unwrap().to_string(),
                        commandline: conf.find("commandline").unwrap().as_array().unwrap().iter().map(|j| j.as_string().unwrap().to_string()).collect(),
                    })
                }).collect::<Vec<_>>());
            (lang.to_string(), nsmap)
        }).collect::<Vec<_>>());
    let decoded = Config { datadir: datadir, runner: runner, compilers: compilers };

    unsafe {
        use std::mem::transmute;
        DATA = transmute(Box::new(decoded));
    }
}

fn get<'a>() -> &'a Config {
    unsafe {
        if DATA == (0 as *const Config) {
            panic!("config not initialized");
        }
        &*DATA
    }
}
