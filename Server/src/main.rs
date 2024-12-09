mod protos;
use anyhow::Result;
use protobuf::{EnumOrUnknown, Message, MessageField};
use protos::message::server_command::{State, StateChange};
use protos::message::ServerCommand;
use tokio::net::TcpListener;
use axum::routing::get;
use axum::Router;

#[tokio::main]
async fn main() -> Result<()> {
    let port = 8080;
    let listener = TcpListener::bind(format!("0.0.0.0:{port}")).await?;

    let app = Router::new()
        .route("/", get(home))
        .route("/protobuf", get(data));

    axum::serve(listener, app).await?;

    Ok(())
}

async fn data() -> Vec<u8> {
    println!("received request");
    let mut message  = ServerCommand::new();
    let mut state = StateChange::new();
    state.state = EnumOrUnknown::new(State::IDLE);

    message.stateChange = MessageField::some(state);

    message.write_to_bytes().unwrap()
}

async fn home() -> &'static str {
    "server"
}
