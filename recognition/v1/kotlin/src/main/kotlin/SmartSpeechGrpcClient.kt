import com.google.protobuf.ByteString
import com.google.protobuf.Duration
import io.grpc.CallOptions
import io.grpc.netty.shaded.io.grpc.netty.GrpcSslContexts
import io.grpc.netty.shaded.io.grpc.netty.NettyChannelBuilder
import io.grpc.netty.shaded.io.netty.handler.ssl.util.InsecureTrustManagerFactory
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.flow
import ru.sberdevices.smartspeech.recognition.v1.Recognition
import ru.sberdevices.smartspeech.recognition.v1.SmartSpeechGrpcKt
import java.io.Closeable
import java.util.concurrent.TimeUnit

class SmartSpeechGrpcClient(private val host: String, private val getToken: () -> String) : Closeable {

    private val sslCtx = GrpcSslContexts.forClient().trustManager(InsecureTrustManagerFactory.INSTANCE).build()
    private val grpcChannel = NettyChannelBuilder.forTarget(host).sslContext(sslCtx).build()

    data class RecognitionResult(val text: String, val eou: Boolean)

    override fun close() {
        grpcChannel.shutdown().awaitTermination(5, TimeUnit.SECONDS)
    }

    class Connection(private val stub: SmartSpeechGrpcKt.SmartSpeechCoroutineStub) {
        var format = "pcm"
        var sampleRate = 16000
        var enableMultiUtterance = false
        var enablePartialResults = false
        var model = "general"

        suspend fun startRecognizing(input: Flow<ByteArray>): Flow<RecognitionResult> = flow {
            val requestsFlow = flow<Recognition.RecognitionRequest> {

                val audioEncoding = when (format) {
                    "pcm" -> Recognition.RecognitionOptions.AudioEncoding.PCM_S16LE
                    "opus" -> Recognition.RecognitionOptions.AudioEncoding.OPUS
                    else -> throw IllegalArgumentException("Wrong audio encoding ${format}. \"pcm\", \"opus\" expected")
                }

                val options = Recognition.RecognitionOptions.newBuilder()
                    .setSampleRate(sampleRate)
                    .setModel(model)
                    .setAudioEncoding(audioEncoding)
                    .setNoSpeechTimeout(Duration.newBuilder().setSeconds(15).build())
                    .setEnableMultiUtterance(enableMultiUtterance)
                    .setEnablePartialResults(enablePartialResults)
                    .build()
                emit(
                    Recognition.RecognitionRequest.newBuilder().setOptions(options).build()
                )
                input.collect { input ->
                    val protoMessage =
                        Recognition.RecognitionRequest.newBuilder().setAudioChunk(ByteString.copyFrom(input)).build()
                    emit(protoMessage)
                }
            }
            val responsesFlow = stub.recognize(requestsFlow)
            responsesFlow.collect { response ->
                emit(RecognitionResult(response.getResults(0).text, response.eou))
            }
        }
    }

    fun createConnection(): Connection {
        val token = getToken()
        val callOptions = CallOptions.DEFAULT.withCallCredentials(BearerToken(token))
        val stub = SmartSpeechGrpcKt.SmartSpeechCoroutineStub(grpcChannel, callOptions)
        return Connection(stub)
    }
}