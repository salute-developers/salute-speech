# Интеграция с телефонией

Вы можете использовать плагины UniMRCP для работы с сервисом SaluteSpeech в телефонии.

[UniMRCP](https://www.unimrcp.org) — это клиент-серверная реализация протокола MRCP.

[SaluteSpeech](https://developers.sber.ru/docs/ru/salutespeech/overview) - сервисы распознавания и синтеза речи на технологиях Сбера.

В плагинах реализованы базовые MRCP-методы:

1. Для распознавания:
    - SET-PARAMS,
    - DEFINE-GRAMMAR,
    - RECOGNIZE,
    - STOP.
2. Для синтеза:
    - SET-PARAMS,
    - SPEAK,
    - BARGE_IN_OCCURRED,
    - STOP.

Вы можете собрать плагины и запустить сервер UniMRCP с помощью Docker и команд `make`.

## Сборка плагинов

Для сборки плагинов используется `bazel`, поэтому сначала подготовьте сборочный образ:

```
> $ make bazel-image
```

После того как сборочный образ готов, необходимо собрать плагины:

```
> $ make build-plugins
```

Плагины скопируются в директорию `output`.

## Запуск сервера UniMRCP

Подготовьте Docker image для UniMRCP сервиса:

```
> $ make unimrcp-image
```

Чтобы подготовить конфигурацию, создайте файл `.env`:

```bash 
SMARTSPEECH_USER_ID=<идентификатор пользователя>
SMARTSPEECH_SECRET=<пароль пользователя>
SMARTSPEECH_SCOPE=SBER_SPEECH
```

:::note
Невозможно запустить два плагина одновременно. Вы можете включить или выключить плагин в файле `package/smartspeech-plugins-config.xml`
:::

Чтобы запустить Docker-контейнер с  UniMRCP-сервисом, выполните команду:

```
> $ make start-test-server
```

Теперь вы можете запустить задачи на синтез или распознования в зависимости от плагина сервера.

1. Текст для синтеза находится в файле `package/speak.xml`. Команда для синтеза:

```
> $ make run-synth
```

2. Звук для распознавания находится в файле `package/one-8kHz.pcm`. Команда для распознавания:

```
> $ make run-recog
```
