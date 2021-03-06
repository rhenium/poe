extern crate byteorder;
extern crate iron;
extern crate router;
extern crate logger;
extern crate uuid;
extern crate time;
extern crate rustc_serialize;

use iron::prelude::*;
use iron::{AfterMiddleware, status};
use iron::modifier::Modifier;
use router::Router;
use logger::Logger;
use rustc_serialize::json::ToJson;
use std::io::Read;

use std::net::*;
use std::collections::BTreeMap;
use std::collections::HashMap;
use std::ascii::AsciiExt;

mod error;
use error::RequestError;
mod config;
#[macro_use] mod utils;
mod snippet;
mod compiler;
mod run_result;
use snippet::Snippet;

impl Modifier<Response> for Snippet {
    fn modify(self, resp: &mut Response) {
        self.render().to_string().modify(resp);
    }
}

fn read_post_form(req: &mut Request) -> Result<HashMap<String, String>, RequestError> {
    let mut body = String::new();
    if let Err(_) = req.body.read_to_string(&mut body) {
        return Err(RequestError::BadRequest);
    }

    Ok(try!(utils::parse_post(&body)))
}

fn snippet_create(req: &mut Request) -> IronResult<Response> {
    let form = try!(read_post_form(req));
    match (form.get("lang"), form.get("code")) {
        (Some(lang_), Some(code)) => {
            let lang = lang_.to_ascii_lowercase();
            let snip = try!(Snippet::create(&lang, &code));
            Ok(Response::with((status::Created, snip)))
        },
        _ => Err(RequestError::BadRequest.into())
    }
}

fn snippet_show(req: &mut Request) -> IronResult<Response> {
    let router = req.extensions.get::<Router>().unwrap();
    match router.find("sid") {
        Some(sid) => Ok(Response::with((status::Ok, try!(Snippet::find(sid))))),
        None => Err(RequestError::BadRequest.into())
    }
}

fn snippet_run(req: &mut Request) -> IronResult<Response> {
    let form = try!(read_post_form(req));
    match (form.get("sid"), form.get("cid")) {
        (Some(sid), Some(cid)) => {
            let snip = try!(Snippet::find(&sid));
            let comp = try!(config::compiler(&snip.metadata.lang, &cid).ok_or(RequestError::NotFound));
            try!(comp.run(&snip));
            Ok(Response::with((status::Ok, run_result::open_render(&snip, &comp).to_string())))
        },
        _ => Err(RequestError::BadRequest.into())
    }
}

fn results_stdout(req: &mut Request) -> IronResult<Response> {
    let router = req.extensions.get::<Router>().unwrap();
    match (router.find("sid"), router.find(":cid")) {
        (Some(sid), Some(cid)) => {
            let snip = try!(Snippet::find(sid));
            let comp = try!(config::compiler(&snip.metadata.lang, cid).ok_or(RequestError::NotFound));
            let stdout = run_result::read_stdout_raw(&snip, &comp);
            Ok(Response::with((status::Ok, stdout)))
        },
        _ => Err(RequestError::BadRequest.into())
    }
}

struct Recoverer;
impl AfterMiddleware for Recoverer {
    fn catch(&self, _: &mut Request, err: IronError) -> IronResult<Response> {
        match err.response.status {
            Some(st) => {
                let mut data = BTreeMap::new();
                data.insert("error".into(), err.to_string());
                Ok(Response::with((st, data.to_json().to_string())))
            },
            _ => Err(err)
        }
    }
}

fn main() {
    config::load();

    let mut router = Router::new();
    router.get("/api/snippet/:sid", snippet_show);
    router.post("/api/snippet/new", snippet_create);
    router.post("/api/snippet/run", snippet_run);
    router.get("/api/snippet/:sid/:cid/stdout", results_stdout);

    let (logger_before, logger_after) = Logger::new(None);

    let mut chain = Chain::new(router);
    chain.link_before(logger_before);
    chain.link_after(Recoverer);
    chain.link_after(logger_after);

    let host = SocketAddrV4::new(Ipv4Addr::new(127, 0, 0, 1), 3000);
    println!("listening on http://{}", host);
    Iron::new(chain).http(host).unwrap();
}
