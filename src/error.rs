use std::error::Error;
use std::fmt;
use std::io;
use rustc_serialize::json;
use iron;

#[derive(Debug)]
pub enum PoeError {
    IOError(io::Error),
    JSONError(json::DecoderError),
    Message(String),
}

impl fmt::Display for PoeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            PoeError::IOError(ref e) => write!(f, "std::io error: {}", e),
            PoeError::JSONError(ref e) => write!(f, "json error: {}", e),
            PoeError::Message(ref s) => write!(f, "mount error: {}", s),
        }
    }
}

impl Error for PoeError {
    fn description(&self) -> &str {
        match *self {
            PoeError::IOError(ref e) => e.description(),
            PoeError::JSONError(ref e) => e.description(),
            PoeError::Message(ref s) => s,
        }
    }
}

impl From<io::Error> for PoeError {
    fn from(e: io::Error) -> Self {
        PoeError::IOError(e)
    }
}

impl From<String> for PoeError {
    fn from(s: String) -> Self {
        PoeError::Message(s)
    }
}

impl From<&'static str> for PoeError {
    fn from(s: &'static str) -> Self {
        PoeError::Message(s.to_string())
    }
}

impl From<json::DecoderError> for PoeError {
    fn from(e: json::DecoderError) -> Self {
        PoeError::JSONError(e)
    }
}

#[derive(Debug)]
pub enum RequestError {
    BadRequest,
    NotFound,
    InternalError(PoeError),
}

impl fmt::Display for RequestError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            RequestError::BadRequest => write!(f, "bad request"),
            RequestError::NotFound => write!(f, "not found"),
            RequestError::InternalError(ref e) => write!(f, "poe: {}", e.description()),
        }
    }
}

impl Error for RequestError {
    fn description(&self) -> &str {
        match *self {
            RequestError::BadRequest => "bad request",
            RequestError::NotFound => "not found",
            RequestError::InternalError(ref e) => e.description(),
        }
    }
}

impl From<RequestError> for iron::error::IronError {
    fn from(e: RequestError) -> Self {
        let modi = match e {
            RequestError::BadRequest => (iron::status::BadRequest),
            RequestError::NotFound => (iron::status::NotFound),
            _ => (iron::status::InternalServerError),
        };
        iron::error::IronError::new(e, modi)
    }
}

impl From<PoeError> for RequestError {
    fn from(e: PoeError) -> Self {
        RequestError::InternalError(e)
    }
}
