const ticks_size = 16;
const labels_size = 20;
const ticks_color = "#CBD5E1";
const line_color = "#3FAFE8";
const padding = 30;

// Инициализация графика
const ctx = document.getElementById('tempChart').getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Температура (°C)',
            data: [],
            borderColor: line_color
        }]
    },
    
    options: {
        responsive: true,
        maintainAspectRatio: false,

        layout: {
            padding: {
                left: padding,
                right: padding,
                top: padding,
                bottom: padding
            }
        },

        scales: {
            x: {
                title: {
                    display: true,
                    text: 'Время',
                    color: ticks_color,
                    font: {
                        size: labels_size,
                        weight: 'bold'
                    }
                },
                ticks: {
                    color: ticks_color,
                    font: {
                        size: ticks_size
                    }
                }
            },

            y: {
                title: {
                    display: true,
                    text: 'Температура (°C)',
                    color: ticks_color,
                    font: {
                        size: labels_size,
                        weight: 'bold'
                    }
                },
                ticks: {
                    color: ticks_color,
                    font: {
                        size: ticks_size
                    }
                }
            }
        },

        plugins: {
            legend: {
                display: false
            }
        }
    }
});

// Обновление текущей температуры
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
            document.getElementById('current').textContent = '--.--';
        });
}

// Загрузка данных за последнюю минуту
function fetchDataLastMinute() {
    // Форматируем временные метки в ISO 8601
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

// Запрос текущей температуры и обновление графика
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

// Запускаем сразу и затем каждую секунду
updateCurrentTemp();
setInterval(updateCurrentTemp, 1000);

// Загружаем данные сразу и потом обновляем каждую секунду
fetchDataLastMinute();
setInterval(fetchAndAddPoint, 1000);