import io.grpc.CallCredentials
import io.grpc.Context
import io.grpc.Metadata
import java.util.concurrent.Executor

class Constants {
    companion object {
        val bearerType = "Bearer"
        val authMetadataKey = Metadata.Key.of("Authorization", Metadata.ASCII_STRING_MARSHALLER)
        val clientIdContextKey = Context.key<String>("clientId")
    }
}

class BearerToken(private var token: String) : CallCredentials() {
    override fun applyRequestMetadata(requestInfo: RequestInfo, executor: Executor, metadataApplier: MetadataApplier) {
        executor.execute {
            val headers = Metadata()
            headers.put(Constants.authMetadataKey, "${Constants.bearerType} $token")
            metadataApplier.apply(headers)
        }
    }

    override fun thisUsesUnstableApi() {}
}