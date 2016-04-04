use rustc_serialize::json::{self, ToJson, Json};
use std::collections::BTreeMap;
use std::fs;
use time;
use error::PoeError;
use std::io::{Read, Write};
use run_result;
use config;
use utils;

#[derive(RustcDecodable, RustcEncodable, Debug)]
pub struct SnippetMetadata {
    pub lang: String,
    pub created: i64,
}

#[derive(Debug)]
pub struct Snippet {
    pub id: String,
    pub metadata: SnippetMetadata,
}

impl Snippet {
    pub fn create(lang: &str, code: &str) -> Result<Snippet, PoeError> {
        if config::compilers(&lang).is_none() {
            return Err(PoeError::NotFound);
        }
        let id = utils::uuid();
        let created = time::now_utc().to_timespec().sec;

        let snip_meta = SnippetMetadata { lang: lang.to_string(), created: created };
        let snip = Snippet { id: id, metadata: snip_meta };

        fs::create_dir(snip.basedir())?;
        fs::create_dir(snip.basedir() + "/results")?;
        let mut metafile = fs::File::create(snip.basedir() + "/metadata.json")?;
        metafile.write(json::encode(&snip.metadata).unwrap().as_bytes())?;
        let mut codefile = fs::File::create(snip.code_file())?;
        codefile.write(code.as_bytes())?;

        Ok(snip)
    }

    pub fn find(id: &str) -> Result<Snippet, PoeError> {
        let metafile_path = config::datadir() + "/snippets/" + &id + "/metadata.json";
        let mut metafile = fs::File::open(metafile_path)?;
        let mut encoded = String::new();
        metafile.read_to_string(&mut encoded)?;
        Ok(Snippet { id: id.to_string(), metadata: json::decode(&encoded)? })
    }

    pub fn code_file(&self) -> String {
        self.basedir() + "/code"
    }

    pub fn basedir(&self) -> String {
        config::datadir() + "/snippets/" + &self.id
    }

    pub fn render(&self) -> Json {
        let mut map = BTreeMap::new();
        map.insert("id".to_string(), self.id.to_json());
        map.insert("lang".to_string(), self.metadata.lang.to_json());
        map.insert("created".to_string(), self.metadata.created.to_json());
        map.insert("code".to_string(), self.read_code().to_json());
        map.insert("results".to_string(), self.render_results());
        Json::Object(map)
    }

    fn read_code(&self) -> String {
        let mut codefile = fs::File::open(config::datadir() + "/snippets/" + &self.id + "/code").unwrap();
        let mut code = String::new();
        codefile.read_to_string(&mut code).unwrap();
        code
    }

    fn render_results(&self) -> Json {
        config::compilers(&self.metadata.lang).unwrap().iter().map(|kv| {
            run_result::open_render(&self, &kv.1)
        }).collect::<Vec<_>>().to_json()
    }
}
