use std::{sync::Arc, time::Duration};

use axum::{body::Bytes, extract::{Path, State}};
use dashmap::DashMap;
use protobuf::{EnumOrUnknown, Message};
use tokio::{sync::{mpsc, oneshot}, time::timeout};
use tracing::error;
use anyhow::{Context, Result};

use crate::{protos::message::{server_command::{self}, RomiData, ServerCommand}, ServerState};

pub type RomiStore = DashMap<u8, RomiCommander>;

type Callback = oneshot::Sender<RomiData>;

pub struct RomiCommander {
    commands: mpsc::Receiver<(ServerCommand, Callback)>,
    callback: Option<Callback>,
}

pub async fn next_state(
    Path(id): Path<u8>,
    state: State<Arc<ServerState>>,
    data: Bytes) -> Vec<u8> {

    match update_state(state, id, data).await {
        Ok(command) => {
            command
        }
        Err(e) => {
            error!("update state fail: {e}");
            let mut command = ServerCommand::new();
            command.state = Some(EnumOrUnknown::new(server_command::State::IDLE));
        
            command
        }
    }.write_to_bytes().unwrap()
}

async fn update_state(
    state: State<Arc<ServerState>>,
    id: u8,
    data: Bytes) -> Result<ServerCommand> {
    let romidata = RomiData::parse_from_bytes(&data)?;

    let mut romi = state.romis.get_mut(&id).context("romi not in map")?;

    if let Some(callback)  = romi.callback.take() {
        callback.send(romidata).ok().context("callback dead")?;
    }

    // fun solution to an invented problem
    let (command, callback) = timeout(
        Duration::from_millis(500), 
        romi.commands.recv()).await
            .context("command channel closed")?
            .context("command timeout")?;

    romi.callback = Some(callback);

    Ok(command)
}
