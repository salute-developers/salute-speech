import com.xenomachina.argparser.ArgParser
import com.xenomachina.argparser.default
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.runBlocking
import java.io.File

class CommandLineArgs(parser: ArgParser) {
    val host by parser.storing("--host", help = "smartspeech host").default("smartspeech.sber.ru")
    val token by parser.storing("--token", help = "smartspeech access token")
    val inputFile by parser.storing("--in", help = "input audio file")
    val sampleRate by parser.storing("--sr", help = "input file sample rate") { toInt() }.default { 16000 }
    val format by parser.storing("--format", help = "input file format (pcm, opus)"). default("pcm")
}

fun main(args: Array<String>) = runBlocking {
    val parsedArgs = ArgParser(args).parseInto(::CommandLineArgs)
    parsedArgs.run {
        println(
            """ recognize: 
            host = $host
            token = $token
            inputFile = $inputFile
            format = $format
            sampleRate = $sampleRate
        """.trimIndent()
        )
    }

    val smartSpeechGrpcClient = SmartSpeechGrpcClient(parsedArgs.host) { parsedArgs.token }
    val smartSpeechConnection = smartSpeechGrpcClient.createConnection().apply {
        sampleRate = parsedArgs.sampleRate
        format = parsedArgs.format
    }

    val readFileFlow = flow {
        File(parsedArgs.inputFile).inputStream().use { input ->
            while (true) {
                val fileBuffer = ByteArray(4096)
                if (input.read(fileBuffer) <= 0) break
                emit(fileBuffer)
                delay(100)
            }
        }
    }

    val recognitionResultsFlow = smartSpeechConnection.startRecognizing(readFileFlow)
    recognitionResultsFlow.collect { result ->
        println(">> ${result.text}")
    }
}