mod protos;
use anyhow::Result;
use protobuf::{EnumOrUnknown, Message, MessageField};
use protos::message::server_command::{State, StateChange};
use protos::message::ServerCommand;
use tokio::net::TcpListener;
use axum::routing::get;
use axum::Router;
use tracing::info;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

#[tokio::main]
async fn main() -> Result<()> {
    // logging
    let stdout = tracing_subscriber::fmt::layer()
        .compact()
        .with_file(true)
        .with_target(true);
    let reg = tracing_subscriber::registry().with(stdout);
    reg.try_init()?;

    let port = 8080;
    let listener = TcpListener::bind(format!("0.0.0.0:{port}")).await?;

    let app = Router::new()
        .route("/", get(home))
        .route("/protobuf", get(data));

    axum::serve(listener, app).await?;

    Ok(())
}

async fn data() -> Vec<u8> {
    info!("received request");
    let mut message  = ServerCommand::new();
    let mut state = StateChange::new();

    state.baseSpeed = 10.;
    state.state = EnumOrUnknown::new(State::LINING);

    message.stateChange = MessageField::some(state);

    message.write_to_bytes().unwrap()
}

async fn home() -> &'static str {
    "server"
}
