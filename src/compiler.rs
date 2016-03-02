use rustc_serialize::json::{ToJson, Json};
use std::collections::BTreeMap;
use std::process::Command;
use error::PoeError;
use config;
use run_result;
use snippet::Snippet;

#[derive(Debug)]
pub struct Compiler {
    pub id: String,
    pub lang: String,
    pub version: String,
    pub commandline: Vec<String>,
}

impl Compiler {
    pub fn run(&self, snippet: &Snippet) -> Result<(), PoeError> {
        let output = try!(Command::new(config::runner())
                          .arg(config::basedir())
                          .arg(self.overlay_path())
                          .arg(snippet.code_file())
                          .args(&self.commandline)
                          .output());
        run_result::parse_and_save(&snippet, &self, output)
    }

    pub fn overlay_path(&self) -> String {
        format!("{}/env/{}/{}", config::datadir(), &self.lang, &self.id)
    }

    pub fn render(&self) -> Json {
        let mut map = BTreeMap::new();
        map.insert("id".to_string(), self.id.to_json());
        map.insert("lang".to_string(), self.lang.to_json());
        map.insert("version".to_string(), self.version.to_json());

        Json::Object(map)
    }
}
