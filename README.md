# obfs0 - Наипростейший TCP-обфускатор

Клиент-серверный обфускатор для TCP-соединений. Маскирует трафик под HTTP запросы/ответы.

# Принцип работы

```
клиент <-> obfs0 --client   <~~~~~~~~~~>   obfs0 --server <-> сервер
                              интернет
```

Для обфусцирования трафика клиент и сервер `obfs0` притворяются веб браузером и HTTP сервером.
Коммуникация происходит следующим образом:
1. Клиент посылает HTTP запрос, передавая в заголовке `ETag` ключ шифрования.
2. Сервер отвечает `200 OK`, в заголовках передаёт случайный `Content-Length`.
   - В случае неправильного запроса сервер отвечает редиректом на `example.com`.
3. Клиент и сервер начинают отправлять друг другу зашифрованный трафик.

Трафик шифруется обычным шифром виженера, длина ключа равна 4 байта. По сути трафик просто ксорится с 4 байтами ключа.
Сделано это лишь для сокрытия сигнатур протоколов, таких как `socks5`. Цели шифрования не стоит.

Изначально `obfs0` написан для использования в связке с socks5 прокси.

# Использование

Запуск в качестве сервера
```
obfs0 --server --listen listen_ip:port target_ip:port

# Пример: перенаправление трафика на локальный socks5 прокси
obfs0 --server --listen 0.0.0.0:80 127.0.0.1:1080

# или
obfs0 127.0.0.1:1080
```

Запуск в качестве клиента
```
obfs0 --client --listen listen_ip:port server_ip:port

# Пример: начать слушать порт 1080 и подключиться к серверу 11.22.33.44:80
obfs0 --client --listen 0.0.0.0:1080 11.22.33.44:80
```

-------

# obfs0 - A really simple TCP obfuscator

Client-server obfuscator for TCP connections. Masks traffic as HTTP requests/responses.

# How it works

```
client <-> obfs0 --client   <~~~~~~~~~~>   obfs0 --server <-> server
                              internet
```

To obfuscate traffic, the `obfs0` client and server pretend to be a web browser and an HTTP server. The communication happens as follows:

1. The client sends an HTTP request, passing the encryption key in the `ETag` header.
2. The server responds with `200 OK` and sends a random `Content-Length` in the headers.
   - If the request is incorrect, the server responds with a redirect to `example.com`.
3. The client and server begin to exchange encrypted traffic.

The traffic is encrypted using a basic Vigenère cipher, with a key length of 4 bytes. Essentially, the traffic is simply XORed with the 4-byte key. This is done solely to hide protocol signatures, such as `socks5`. Encryption is not the goal.

Initially, `obfs0` was written to be used in conjunction with a socks5 proxy.

# Usage

Runnung as server
```
obfs0 --server --listen listen_ip:port target_ip:port

# Example: routing traffic to local socks5 proxy
obfs0 --server --listen 0.0.0.0:80 127.0.0.1:1080

# or
obfs0 127.0.0.1:1080
```

Runnung as client
```
obfs0 --client --listen listen_ip:port server_ip:port

# Example: bind local port 1080 to remote server 11.22.33.44:80
obfs0 --client --listen 0.0.0.0:1080 11.22.33.44:80
```

