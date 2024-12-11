mod map;
mod protos;
mod romi;
mod view;
use std::net::SocketAddr;
use std::path::PathBuf;
use std::str::FromStr;
use std::sync::Arc;

use anyhow::Result;
use axum::routing::{any, get, post};
use axum::Router;
use map::{obstacle, remove_obstacle, Map};
use protobuf::{EnumOrUnknown, Message};
use protos::message::server_command::{self, State};
use protos::message::ServerCommand;
use romi::{next_state, RomiCommander, RomiStore};
use tokio::net::TcpListener;
use tokio::sync::mpsc::{self, Sender};
use tokio::sync::RwLock;
use tracing::info;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};
use tower_http::services::ServeDir;
use view::ws_handler;

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

    let mut map = Map::new();
    map.insert_obstacle((0,2));
    map.insert_obstacle((1,3));
    map.insert_obstacle((2,4));

    let state = ServerState {
        romis: Default::default(),
        commanders: sender,
        map: RwLock::new(map),
    };

    let page = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("public");
    let state = Arc::new(state);
    let app = Router::new()
        .fallback_service(ServeDir::new(page).append_index_html_on_directories(true))
        //.route("/", get(home))
        .route("/protobuf", get(data))
        .route("/protobuf", post(data))
        .route("/obstacle", post(obstacle))
        .route("/obstacle", post(remove_obstacle))
        .route("/positions", any(ws_handler))
        .route("/nextState/:id", post(next_state))
        .with_state(state.clone())
        .into_make_service_with_connect_info::<SocketAddr>();

    tokio::spawn(async {
        axum::serve(listener, app).await.unwrap();
    });

    let mut romi = romis.recv().await.unwrap();

    //loop {
    //    let mut command = ServerCommand::new();
    //    command.state = Some(EnumOrUnknown::new(server_command::State::SEARCHING));
    //    romi.execute(command).await.unwrap();
    //}
    let map = &state.map;

    romi.route(map, (0, 4)).await?;

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
