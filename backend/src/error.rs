use std::error::Error;
use std::fmt;
use std::io;
use rustc_serialize::json;
use iron;

#[derive(Debug, Clone)]
pub enum PoeError {
    NotFound,
    Unknown(String),
}

impl fmt::Display for PoeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            PoeError::NotFound => write!(f, "not found"),
            PoeError::Unknown(ref s) => write!(f, "error: {}", s),
        }
    }
}

impl Error for PoeError {
    fn description(&self) -> &str {
        match *self {
            PoeError::NotFound => "not found",
            PoeError::Unknown(ref s) => s,
        }
    }
}

impl From<io::Error> for PoeError {
    fn from(e: io::Error) -> Self {
        match e.kind() {
            io::ErrorKind::NotFound => PoeError::NotFound,
            _ => PoeError::Unknown(e.description().to_string())
        }
    }
}

impl From<json::DecoderError> for PoeError {
    fn from(e: json::DecoderError) -> Self {
        PoeError::Unknown(e.description().to_string())
    }
}

impl From<String> for PoeError {
    fn from(s: String) -> Self {
        PoeError::Unknown(s)
    }
}

impl<'a> From<&'a str> for PoeError {
    fn from(s: &'a str) -> Self {
        PoeError::Unknown(s.to_string())
    }
}

#[derive(Debug, Clone)]
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
        match e {
            PoeError::NotFound => RequestError::NotFound,
            _ => RequestError::InternalError(e),
        }
    }
}

impl From<PoeError> for iron::error::IronError {
    fn from(e: PoeError) -> Self {
        RequestError::from(e).into()
    }
}
