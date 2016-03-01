#![feature(plugin)]
#![feature(custom_derive)]
#![plugin(serde_macros)]

extern crate byteorder;
extern crate num;
extern crate iron;
extern crate mount;
extern crate staticfile;
extern crate router;
extern crate serde;
extern crate serde_json;
extern crate hyper;
extern crate uuid;
extern crate time;
extern crate rustc_serialize;

use iron::prelude::*;
use iron::{status, headers, modifiers};
use iron::{BeforeMiddleware, AfterMiddleware, typemap};
use mount::Mount;
use staticfile::Static;
use router::Router;
use rustc_serialize::json::{self, ToJson, Json};
use std::collections::HashMap;
use std::io::{Read,Write};
use std::iter::FromIterator;

use std::env;
use std::net::*;
use std::path::Path;
use std::collections::BTreeMap;
use std::ascii::AsciiExt;
use std::error::Error;

mod error;
use error::PoeError;
use error::RequestError;
mod config;
#[macro_use] mod utils;
mod snippet;
mod compiler;
mod run_result;
use snippet::Snippet;
use compiler::Compiler;

fn snippet_create(req: &mut Request) -> IronResult<Response> {
    let mut body = String::new();
    let form = try!(req.body.read_to_string(&mut body)
        .map(|_| utils::parse_post(body))
        .map_err(|e| RequestError::from(PoeError::from(e.to_string()))));

    match (form.get("lang"), form.get("code")) {
        (Some(lang_), Some(code)) => {
            let lang = lang_.to_ascii_lowercase();
            match Snippet::create(&lang, &code) {
                Ok(snip) => Ok(Response::with((status::Created, snip.render().to_string()))),
                Err(e) => Err(RequestError::InternalError(e).into())
            }
        },
        _ => Err(RequestError::BadRequest.into())
    }
}

fn snippet_show(req: &mut Request) -> IronResult<Response> {
    let router = req.extensions.get::<Router>().unwrap();
    let osnip = router.find("snippet_id").and_then(|sid| Snippet::find(sid).ok());

    match osnip {
        Some(snip) => Ok(Response::with((status::Ok, snip.render().to_string()))),
        None => Ok(Response::with((status::NotFound)))
    }
}

fn snippet_run(req: &mut Request) -> IronResult<Response> {
    let mut body = String::new();
    let form = try!(req.body.read_to_string(&mut body)
        .map(|_| utils::parse_post(body))
        .map_err(|e| RequestError::from(PoeError::from(e))));

    match (form.get("sid"), form.get("cid")) {
        (Some(sid), Some(cid)) => {
            let snip = try!(Snippet::find(&sid).map_err(|e|RequestError::NotFound));
            let comp = try!(config::compiler(&snip.metadata.lang, &cid).ok_or(RequestError::NotFound));
            try!(comp.run(&snip).map_err(|e|RequestError::from(e)));
            Ok(Response::with((status::Ok, run_result::open_render(&snip, &comp).to_string())))
        },
        _ => Err(RequestError::BadRequest.into())
    }
}

struct Recoverer;
impl AfterMiddleware for Recoverer {
    fn catch(&self, _: &mut Request, err: IronError) -> IronResult<Response> {
        match err.response.status {
            Some(st) => {
                let mut data = BTreeMap::<String, _>::new();
                data.insert("error".into(), err.to_string());
                Ok(Response::with((st, json::encode(&data).unwrap())))
            },
            _ => Err(err)
        }
    }
}

fn main() {
    config::load();

    let mut router = Router::new();
    router.get("/api/snippet/:snippet_id", snippet_show);
    router.post("/api/snippet/new", snippet_create);
    router.post("/api/snippet/run", snippet_run);

    let mut mount = Mount::new();
    mount.mount("/", router);
    mount.mount("/static", Static::new(Path::new("./static/")));

    let mut middleware = Chain::new(mount);
    middleware.link_after(Recoverer);

    let host = SocketAddrV4::new(Ipv4Addr::new(127, 0, 0, 1), 3000);
    println!("listening on http://{}", host);
    Iron::new(middleware).http(host).unwrap();
}
