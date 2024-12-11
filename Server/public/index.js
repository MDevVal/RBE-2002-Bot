const GRID_ROWS = 10;
const GRID_COLS = 10;
let cellSize = 50;
let gridWidth = 500;
let gridHeight = 500;

const grid = document.getElementById('grid');
const legendContent = document.getElementById('legend-content');
const entities = {};
const knownTypes = {}; 
let currentGoalKey = null;

function recalcGridSize() {
    const marginFactor = 0.9;
    const windowWidth = window.innerWidth - 200;
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
    grid.style.setProperty('--cell-size', cellSize + 'px');
    grid.style.backgroundSize = `${cellSize}px ${cellSize}px`;

    for (const key in entities) {
        const ent = entities[key];
        if (ent.x != null && ent.y != null) {
            updateEntityPositionAndSize(ent);
        }
    }
}

function updateEntityPositionAndSize(ent) {
    ent.div.style.left = (ent.x * cellSize) + 'px';
    ent.div.style.top = (ent.y * cellSize) + 'px';
    ent.div.style.width = cellSize + 'px';
    ent.div.style.height = cellSize + 'px';
    ent.div.style.fontSize = (cellSize * 0.4) + 'px';
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
            div.textContent = name;
        } else if (type === 'Obstacle') {
            div.classList.add('obstacle');
            div.textContent = "";
        } else if (type === 'Goal') {
            div.classList.add('goal');
            div.textContent = "";
        }
        grid.appendChild(div);
        entity = { div: div, x: x, y: y, type: type, name: name };
        entities[key] = entity;
        if (!knownTypes[type]) {
            knownTypes[type] = true;
            addToLegend(type, window.getComputedStyle(div).backgroundColor);
        }
    } else {
        entity.x = x;
        entity.y = y;
        if (type === 'Romi') {
            entity.div.textContent = name;
        }
    }
    updateEntityPositionAndSize(entity);
}

function addToLegend(type, color) {
    const legendItem = document.createElement('div');
    legendItem.classList.add('legend-item');
    const colorBox = document.createElement('div');
    colorBox.classList.add('legend-color-box');
    colorBox.style.background = color;
    const label = document.createElement('span');
    label.textContent = type;
    legendItem.appendChild(colorBox);
    legendItem.appendChild(label);
    legendContent.appendChild(legendItem);
}

grid.addEventListener('contextmenu', event => {
    event.preventDefault();
});

grid.addEventListener('mousedown', event => {
    const rect = grid.getBoundingClientRect();
    const offsetX = event.clientX - rect.left;
    const offsetY = event.clientY - rect.top;
    const x = Math.floor(offsetX / cellSize);
    const y = Math.floor(offsetY / cellSize);

    if (x < 0 || y < 0 || x >= GRID_COLS || y >= GRID_ROWS) return;

    const obstacleKey = "Obstacle_" + x + "_" + y;
    const goalKey = "Goal_" + x + "_" + y;
    const obstacleExists = !!entities[obstacleKey];
    const goalExists = !!entities[goalKey];

    // Left click: Place obstacle if no obstacle and no goal there
    if (event.button === 0 && !obstacleExists && !goalExists) {
        updateEntity('Obstacle', x, y, x+"_"+y);
        sendAddObstacle(x, y);
    } 
    // Right click: Remove obstacle if one exists
    else if (event.button === 2 && obstacleExists) {
        removeEntity('Obstacle', x, y);
        sendRemoveObstacle(x, y);
    } 
    // Middle click: Set a single goal if no obstacle there (and no other goal in this cell)
    else if (event.button === 1 && !obstacleExists) {
        removeExistingGoal();
        if (!goalExists) {
            const goalName = x+"_"+y;
            updateEntity('Goal', x, y, goalName);
            currentGoalKey = "Goal_" + goalName;
            sendGoal(x, y);
        }
    }
});

function removeEntity(type, x, y) {
    const key = type + "_" + x + "_" + y;
    const entity = entities[key];
    if (entity) {
        grid.removeChild(entity.div);
        delete entities[key];
    }
}

function removeExistingGoal() {
    if (currentGoalKey && entities[currentGoalKey]) {
        const entity = entities[currentGoalKey];
        grid.removeChild(entity.div);
        delete entities[currentGoalKey];
        currentGoalKey = null;
    }
}

function sendAddObstacle(x, y) {
    fetch('/obstacle', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify([x, y])
    }).catch(err => console.error('Error posting obstacle add:', err));
}

function sendRemoveObstacle(x, y) {
    fetch('/remove_obstacle', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify([x, y])
    }).catch(err => console.error('Error posting obstacle remove:', err));
}

function sendGoal(x, y) {
    fetch('/goal', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify([x, y])
    }).catch(err => console.error('Error posting goal:', err));
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

