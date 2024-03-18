console.log('index.js runnin');

document.getElementById('game-form').addEventListener('submit', function(event) {
    var name = document.getElementById('name').value;
    var number = document.getElementById('number').value;

    event.preventDefault();

    var url = 'http://localhost:3000/' + name + '?number=' + number;
    console.log(url);

    console.log('fetching');
    window.location.href = url;
});
