// TODO

// === 1. Функция для обновления текущей температуры ===
function updateCurrentTemp() {
    fetch('/current')
        .then(res => {
            if (!res.ok) throw new Error('Failed to fetch current temp');
            return res.json();
        })
        .then(data => {
            document.getElementById('current').textContent = data.temperature.toFixed(2);
        })
        .catch(err => {
            console.error('Error fetching current temperature:', err);
            document.getElementById('current').textContent = '—';
        });
}

// Запускаем сразу и затем каждую секунду
updateCurrentTemp();
setInterval(updateCurrentTemp, 1000);

// === 2. Инициализация графика ===
const ctx = document.getElementById('tempChart').getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Температура (°C)',
            data: [],
            borderColor: 'rgb(76, 103, 213)',
            tension: 0.1,
            fill: false
        }]
    },
    options: {
        // scales: {
        //     x: {
        //         title: {
        //             display: true,
        //             text: 'Время'
        //         }
        //     },
        //     y: {
        //         title: {
        //             display: true,
        //             text: 'Температура (°C)'
        //         }
        //     }
        // },
        plugins: {
            legend: {
                display: false
            }
        }
    }
});

// === 3. Функция для загрузки данных за последний час ===
function fetchDataLastMinute() {
    // Форматируем временные метки в ISO 8601 без 'T' (лучше совместимость с SQLite)
    const now = new Date();
    const oneMinuteAgo = new Date(now.getTime() - 60 * 1000);

    const start = oneMinuteAgo.toISOString().slice(0, 19).replace('T', ' ');
    const end = now.toISOString().slice(0, 19).replace('T', ' ');

    fetch(`/history?start=${encodeURIComponent(start)}&end=${encodeURIComponent(end)}`)
        .then(res => {
            if (!res.ok) throw new Error('Failed to fetch history');
            return res.json();
        })
        .then(points => {
            const labels = points.map(p => p.timestamp.split(' ')[1]);
            const values = points.map(p => p.temperature);

            chart.data.labels = labels;
            chart.data.datasets[0].data = values;
            chart.update();
        })
        .catch(err => {
            console.error('Error fetching temperature history:', err);
        });
}

// === Запрос текущей температуры и обновление графика ===
function fetchAndAddPoint() {
    fetch('/current')
        .then(res => {
            if (!res.ok) throw new Error('Failed to fetch current temp');
            return res.json();
        })
        .then(data => {
            // Добавляем новую точку
            chart.data.labels.push(new Date().toLocaleTimeString());
            chart.data.datasets[0].data.push(data.temperature);

            // Удаляем самую старую, если превысили лимит
            if (chart.data.labels.length > 60) {
                chart.data.labels.shift();
                chart.data.datasets[0].data.shift();
            }

            chart.update('none');
        })
        .catch(err => {
            console.error('Error fetching temperature:', err);
        });   
}

// Загружаем данные сразу и потом обновляем каждые 1 секунд
fetchDataLastMinute();
setInterval(fetchAndAddPoint, 1000);