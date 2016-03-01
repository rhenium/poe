use num;
use std::io::Cursor;
use byteorder::{ReadBytesExt, WriteBytesExt, BigEndian, LittleEndian};
use compiler::Compiler;
use std::process::Output;
use rustc_serialize::json::{self, ToJson, Json};
use rustc_serialize::base64::{self, ToBase64};
use std::collections::BTreeMap;
use std::fs;
use time;
use uuid;
use error::PoeError;
use std::io::{Read, Write};
use run_result;
use config;
use std::collections::HashSet;
use std::iter::FromIterator;

#[derive(Serialize, Deserialize, RustcDecodable, RustcEncodable, Debug)]
pub struct SnippetMetadata {
    pub lang: String,
    pub created: i64,
}

pub struct Snippet {
    pub id: String,
    pub metadata: SnippetMetadata,
}

impl Snippet {
    pub fn create(lang: &str, code: &str) -> Result<Snippet, PoeError> {
        if config::compilers(&lang).is_none() {
            return Err(PoeError::from("unknown lang"));
        }
        let id = uuid::Uuid::new_v4().to_simple_string();
        let created = time::now_utc().to_timespec().sec;

        let snip_meta = SnippetMetadata {
            lang: lang.to_string(),
            created: created,
        };
        let snip = Snippet {
            id: id,
            metadata: snip_meta,
        };

        try!(fs::create_dir(snip.basedir()));
        try!(fs::create_dir(snip.basedir() + "/results"));
        let mut metafile = try!(fs::File::create(snip.basedir() + "/metadata.json"));
        try!(metafile.write(json::encode(&snip.metadata).unwrap().as_bytes()));
        let mut codefile = try!(fs::File::create(snip.code_file()));
        try!(codefile.write(code.as_bytes()));

        Ok(snip)
    }

    pub fn find(id: &str) -> Result<Snippet, PoeError> {
        let mut metafile = try!(fs::File::open(config::datadir() + "/snippets/" + &id +
                                               "/metadata.json"));
        let mut encoded = String::new();
        try!(metafile.read_to_string(&mut encoded));
        Ok(Snippet {
            id: id.to_string(),
            metadata: try!(json::decode(&encoded)),
        })
    }

    // ToJson を実装するのはアレな気がする
    pub fn render(&self) -> Json {
        let mut map = BTreeMap::new();
        map.insert("id".to_string(), self.id.to_json());
        map.insert("lang".to_string(), self.metadata.lang.to_json());
        map.insert("created".to_string(), self.metadata.created.to_json());
        map.insert("code".to_string(), self.read_code().to_json());
        map.insert("results".to_string(), self.render_results());
        Json::Object(map)
    }

    fn render_results(&self) -> Json {
        config::compilers(&self.metadata.lang).unwrap().iter().map(|kv| run_result::open_render(&self, &kv.1)).collect::<Vec<_>>().to_json()
    }

    pub fn code_file(&self) -> String {
        self.basedir() + "/code"
    }

    pub fn basedir(&self) -> String {
        config::datadir() + "/snippets/" + &self.id
    }

    fn read_code(&self) -> String {
        let mut codefile = fs::File::open(config::datadir() + "/snippets/" + &self.id + "/code")
                               .unwrap();
        let mut code = String::new();
        codefile.read_to_string(&mut code).unwrap();
        code
    }
}