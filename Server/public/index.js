const GRID_ROWS = 10;
const GRID_COLS = 10;
let cellSize = 50;
let gridWidth = 500;
let gridHeight = 500;
const grid = document.getElementById('grid');
const entities = {};

function recalcGridSize() {
    const marginFactor = 0.9;
    const windowWidth = window.innerWidth;
    const windowHeight = window.innerHeight;
    const minDimension = Math.min(windowWidth, windowHeight) * marginFactor;
    const newGridSize = Math.floor(minDimension);
    cellSize = newGridSize / GRID_ROWS;
    gridWidth = newGridSize;
    gridHeight = newGridSize;
    grid.style.width = gridWidth + 'px';
    grid.style.height = gridHeight + 'px';
    grid.style.backgroundImage = `
        linear-gradient(to right, #ddd 1px, transparent 1px),
        linear-gradient(to bottom, #ddd 1px, transparent 1px)
    `;
    grid.style.backgroundSize = `${cellSize}px ${cellSize}px`;
    for (const key in entities) {
        const ent = entities[key];
        if (ent.x != null && ent.y != null) {
            ent.div.style.left = (ent.x * cellSize) + 'px';
            ent.div.style.top = (ent.y * cellSize) + 'px';
            ent.div.style.width = cellSize + 'px';
            ent.div.style.height = cellSize + 'px';
        }
    }
}

window.addEventListener('resize', recalcGridSize);
recalcGridSize();

const socket = new WebSocket("ws://" + location.host + "/positions");
socket.addEventListener('open', function () {
    socket.send('Hello Server!');
});
socket.addEventListener('message', function (event) {
    const msg = JSON.parse(event.data);
    if (msg.Romi) {
        const { x, y, name } = msg.Romi;
        updateEntity('Romi', x, y, name || (x+"_"+y));
    } else if (msg.Obstacle) {
        const { x, y } = msg.Obstacle;
        updateEntity('Obstacle', x, y, x+"_"+y);
    }
});

function updateEntity(type, x, y, name) {
    const key = type + "_" + name;
    let entity = entities[key];
    if (!entity) {
        const div = document.createElement('div');
        div.classList.add('entity');
        if (type === 'Romi') {
            div.classList.add('romi');
        } else {
            div.classList.add('obstacle');
        }
        grid.appendChild(div);
        entity = { div: div, x: x, y: y };
        entities[key] = entity;
    } else {
        entity.x = x;
        entity.y = y;
    }
    entity.div.style.left = (x * cellSize) + 'px';
    entity.div.style.top = (y * cellSize) + 'px';
    entity.div.style.width = cellSize + 'px';
    entity.div.style.height = cellSize + 'px';
}

setTimeout(() => {
    const obj = { hello: "world" };
    const blob = new Blob([JSON.stringify(obj, null, 2)], {
      type: "application/json",
    });
    socket.send(blob);
}, 1000);

setTimeout(() => {
    socket.send('About done here...');
    socket.close(3000, "Closing now!");
}, 3000);

