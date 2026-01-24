// Загрузка текущей температуры
fetch('/current')
    .then(res => res.json())
    .then(data => {
        document.getElementById('current').textContent = data.value;
    });

// Инициализация графика
const ctx = document.getElementById('tempChart').getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Temperature',
            data: []
        }]
    },
    options: {}
});

// Загрузка статистики за период
fetch('/average?start=2023-01-01T00:00&end=2023-01-01T01:00')
    .then(res => res.json())
    .then(data => {
        // Обновление графика
        chart.data.labels.push('Now');
        chart.data.datasets[0].data.push(data.avg);
        chart.update();
    });