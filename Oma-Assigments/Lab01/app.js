const express = require('express');
const app = express();

app.use(express.static('Oma-Assigments/Lab01'));

console.log('app.js runnin')

var num_min = 1;
var num_max = 10;

app.get('/', (req, res) => {
    console.log('GET /')
    res.sendFile(__dirname + '/index.html');
});

app.get('/:name', (req, res) => {
    var num_rand = Math.floor(Math.random() * (num_max - num_min + 1)) + num_min;
    console.log('GET /:name')
    const name = req.params.name;
    const num = parseInt(req.query.number);
    
    console.log('name:', name);
    console.log('num:', num);
    console.log('num_rand:', num_rand);

    if (num < num_min || num > num_max || isNaN(num)) {
        res.status(200).send('Hey ' + name + '!' + ' Read the instructions!');
        }
    else if (num === num_rand) {
        res.status(200).send('Hey ' + name + '!' + ' Congratulations! You guessed the right number!');
    } 
    else {
        res.status(200).send('Hey ' + name + '!' + ' Sorry! The number was ' + num_rand + '. Try again!');
    }
});


app.get('/*/*', (req, res) => {
    res.redirect('http://localhost:3000');
});

const port = process.env.PORT || 3000;
app.listen(port, () => {
    console.log(`Server is listening on port ${port}`);
});