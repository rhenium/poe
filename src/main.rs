#![feature(plugin)]
#![feature(custom_derive)]

extern crate byteorder;
extern crate iron;
extern crate router;
extern crate uuid;
extern crate time;
extern crate rustc_serialize;

use iron::prelude::*;
use iron::{AfterMiddleware, status};
use router::Router;
use rustc_serialize::json::ToJson;
use std::io::Read;

use std::net::*;
use std::collections::BTreeMap;
use std::ascii::AsciiExt;

mod error;
use error::PoeError;
use error::RequestError;
mod config;
#[macro_use] mod utils;
mod snippet;
mod compiler;
mod run_result;
use snippet::Snippet;

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
    router.get("/api/snippet/:snippet_id", snippet_show);
    router.post("/api/snippet/new", snippet_create);
    router.post("/api/snippet/run", snippet_run);

    let mut middleware = Chain::new(router);
    middleware.link_after(Recoverer);

    let host = SocketAddrV4::new(Ipv4Addr::new(127, 0, 0, 1), 3000);
    println!("listening on http://{}", host);
    Iron::new(middleware).http(host).unwrap();
}
