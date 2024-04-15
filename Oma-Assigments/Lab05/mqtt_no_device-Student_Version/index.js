const express = require('express');
const uuid = require('uuid');
const mqtt = require('mqtt');
const path = require('path');
const util = require('util');
const fs = require('fs');
const bodyParser = require('body-parser');
const app = express();
const http = require('http');

const readFile = util.promisify(fs.readFile);
const writeFile = util.promisify(fs.writeFile);
// Create variables for MQTT use here

const topic_to_sub_to = '#';
const clientId = `mqtt_${Math.random().toString(16).slice(3)}`
const mqtt_client_address = '18.198.188.151';
const mqtt_client_port = '21883';
const mqtt_options = {
    clean: true,
    connectTimeout: 40000,
    clientId: clientId,
    reconnectPeriod: 40000,
    rejectUnauthorized: false,
    username: 'SmartIoT',
    password: 'SmartIoTMQTT',
};

app.use(bodyParser.json());
function read(filePath = './message.json') {
    return readFile(path.resolve(__dirname, filePath)).then(data => JSON.parse(data));
}
function write(data, filePath = './message.json') {
    return writeFile(path.resolve(__dirname, filePath), JSON.stringify(data));
}

// create an MQTT instance
const mqtt_client = mqtt.connect(`mqtt://${mqtt_client_address}:${mqtt_client_port}`, mqtt_options);

// Check that you are connected to MQTT and subscribe to a topic (connect event)
mqtt_client.on('connect', () => {
    console.log('Connected to MQTT');
    mqtt_client.subscribe(`${topic_to_sub_to}`); // Subscribe to the topic
});

// handle instance where MQTT will not connect (error event)
mqtt_client.on('error', (error) => {
    console.error('MQTT connection error:', error);
});


/*
Expected format:
{
“topic”: “<whatever your topic is>”,
“msg”: “<whatever you want your message to be>”
}
*/
// Handle when a subscribed message comes in (message event)
mqtt_client.on('message', (topic, message) => {

    try {

        read().then(data => {

        const newMessage = JSON.parse(message.toString());

        if (!newMessage.topic || !newMessage.msg) {
            console.log('Invalid message format');
            return res.status(400).send('Invalid message format'); // mby return something snarkier
        }

        newMessage.id = Math.random().toString(36).substr(2, 9);
        data.push(newMessage); // Add to json file
        write(data);
    }).catch(e => {
        console.log('Error:', e);
        //not doing anything
    });
    } catch (error) {
        console.error('Invalid JSON:', error);
        return;
    }

    console.log('Received message:', message.toString(), 'from topic:', topic);
});

// Route to serve the home page
app.get('/', (req, res) => {
    res.sendFile(path.resolve(__dirname, 'index.html'));
});

// route to serve the JSON array from the file message.json when requested from the home page
app.get('/messages', async (req, res) => {
    try {
        const messages = await read();
        res.json(messages);
    } catch (e) {
        res.sendStatus(500);
    }
});

// Route to serve the page to add a message
app.get('/add', (req, res) => {
    res.sendFile(path.resolve(__dirname, 'message.html'));
});

//Route to show a selected message. Note, it will only show the message as text. No html needed
app.get('/:id', async (req, res) => {
    try {
        const messages = await read();
        const message = messages.find(c => c.id === req.params.id);
        res.send(message);
    } catch (e) {
        res.sendStatus(500);
    }
});

// Route to CREATE a new message on the server and publish to mqtt broker
app.post('/', async (req, res) => {
    try {
        const messages = await read();
        const newMessage = req.body;


        if (!newMessage.topic || !newMessage.msg) {
            console.log('Invalid message format');
            return res.status(400).send('Invalid message format'); // mby return something snarkier
        }


        const topic = newMessage.topic;
        newMessage.id = Math.random().toString(36).substr(2, 9);
        messages.push(newMessage); // Add to json file
        //await write(messages);
        mqtt_client.publish(topic, JSON.stringify(newMessage)); // Publish to MQTT
        res.sendStatus(200);
    } catch (e) {
        res.sendStatus(500);
    }
});

// Route to delete a message by id (Already done for you)

app.delete('/:id', async (req, res) => {
    try {
        const messages = await read();
        write(messages.filter(c => c.id !== req.params.id));
        res.sendStatus(200);
    } catch (e) {
        res.sendStatus(200);
    }
});

// listen to the port
app.listen(3000);