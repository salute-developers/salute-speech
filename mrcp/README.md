<h1 align="center"> ✨ smartspeech-unimrcp ✨ </h1>

Примеры UniMRCP плагинов с реализацией синтеза и распознавания с использованием SmartSpeech.
1. UniMRCP - клиент-серверная реализация протокола MRCP. https://www.unimrcp.org/
2. SmartSpeech - сервисы распознавания и синтеза речи на технологиях Сбера. https://developers.sber.ru/docs/ru/salutespeech/overview

Реализованы базовые MRCP методы. Для распознавания: `SET-PARAMS`, `DEFINE-GRAMMAR`, `RECOGNIZE`, `STOP`. Для синтеза: `SET-PARAMS`, `SPEAK`, `BARGE_IN_OCCURRED`, `STOP`.

<h4 align="center">❗️ Инструкция по сборке и запуску❗️ </h4>

Для удобства и простоты все шаги реализованы с помощью докера и команд make.

##### Сборка:
Для сборки плагинов используется `bazel`, поэтому первый шаг - подготовка сборочного образа
> $ make bazel-image

После того как сборочный образ готов необходимл собрать сами плагины
> $ make build-plugins

Плагины скопируются в директорию output

##### Следующий шаг - запуск сервера unimrcp, он так же запускается в докере.
Подготовка образа для unimrcp сервиса:
> $ make unimrcp-image

Подготовка конфигурации, необходимо создать файл `.env` с 
```bash 
SMARTSPEECH_USER_ID=<...>
SMARTSPEECH_SECRET=<...>
SMARTSPEECH_SCOPE=SBER_SPEECH
```
Запуск сервера

> ❗️ Внимание: в текущем релизе невозможно запустить два плагина одновременно, управлять включением/выключением можно в `package/smartspeech-plugins-config.xml`

> $ make start-test-server

После этого можно запускать задачи на синтез/распознования в зависимости от плагина сервера.
Фраза для синтеза находится в `package/speak.xml`
> $ make run-synth

Звук для распознавания находится `package/one-8kHz.pcm`
> $ make run-recog
