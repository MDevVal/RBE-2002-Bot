
const socket = new WebSocket("ws://" + location.host + "/positions");
const romi = document.getElementById("romi");

socket.addEventListener('open', function (event) {
    socket.send('Hello Server!');
});

socket.addEventListener('message', function (event) {
    console.log('Message from server ', event.data);
    const msg = JSON.parse(event.data);
    console.log(msg);

    // Move the image to the specified coordinates
    var x = msg.x;
    var y = msg.y;

    x = x * 100 + 50;
    y = y + 50;

    romi.style.left = x  + '%';
    romi.style.top = y + '%';
});


setTimeout(() => {
    const obj = { hello: "world" };
    const blob = new Blob([JSON.stringify(obj, null, 2)], {
      type: "application/json",
    });
    console.log("Sending blob over websocket");
    socket.send(blob);
}, 1000);

setTimeout(() => {
    socket.send('About done here...');
    console.log("Sending close over websocket");
    socket.close(3000, "Crash and Burn!");
}, 3000);
