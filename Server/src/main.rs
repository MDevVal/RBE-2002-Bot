mod protos;
mod romi;
mod map;
use std::sync::Arc;

use anyhow::Result;
use map::Map;
use protobuf::{EnumOrUnknown, Message};
use protos::message::server_command::State;
use protos::message::ServerCommand;
use romi::{next_state, RomiCommander, RomiStore};
use tokio::net::TcpListener;
use axum::routing::{get, post};
use axum::Router;
use tokio::sync::mpsc::{self, Sender};
use tokio::sync::RwLock;
use tracing::info;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

struct ServerState {
    romis: RomiStore,
    commanders: Sender<RomiCommander>,
    map: RwLock<Map>,
}

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

    let (sender, mut romis) = mpsc::channel(8);

    let map = Map::new();

    let state = ServerState {
        romis: Default::default(),
        commanders: sender,
        map: RwLock::new(map),
    };

    let state = Arc::new(state);
    let app = Router::new()
        .route("/", get(home))
        .route("/protobuf", get(data))
        .route("/nextState/:id", post(next_state))
        .with_state(state.clone());

    tokio::spawn(async { 
        axum::serve(listener, app).await.unwrap();
    });

    let mut romi = romis.recv().await.unwrap();

    let map = &state.map;

    romi.route(map, (1,1)).await?;
    romi.route(map, (0,0)).await?;
    romi.go_cell(0,1).await?;
    romi.go_cell(1,1).await?;
    romi.go_cell(1,0).await?;
    romi.go_cell(0,1).await?;

    Ok(())
}

async fn data() -> Vec<u8> {
    info!("received request");
    let mut state = ServerCommand::new();

    state.baseSpeed = 10.;
    state.state = Some(EnumOrUnknown::new(State::LINING));

    state.write_to_bytes().unwrap()
}

async fn home() -> &'static str {
    "server"
}
