import Foundation
import LocusQHeadTrackerCore

struct CliOptions {
    let host: String
    let port: UInt16

    static func parse(_ args: [String]) throws -> CliOptions {
        var host = "127.0.0.1"
        var port: UInt16 = 19765

        var i = 1
        while i < args.count {
            let arg = args[i]
            switch arg {
            case "--host":
                i += 1
                guard i < args.count else { throw CliError.missingValue("--host") }
                host = args[i]
            case "--port":
                i += 1
                guard i < args.count else { throw CliError.missingValue("--port") }
                guard let p = UInt16(args[i]) else { throw CliError.invalidValue("--port", args[i]) }
                port = p
            case "--help", "-h":
                throw CliError.help
            default:
                throw CliError.invalidArgument(arg)
            }
            i += 1
        }

        return CliOptions(host: host, port: port)
    }
}

enum CliError: Error, CustomStringConvertible {
    case missingValue(String)
    case invalidValue(String, String)
    case invalidArgument(String)
    case help

    var description: String {
        switch self {
        case .missingValue(let flag):
            return "Missing value for \(flag)."
        case .invalidValue(let flag, let value):
            return "Invalid value \(value) for \(flag)."
        case .invalidArgument(let arg):
            return "Unknown argument: \(arg)."
        case .help:
            return """
            Usage: LocusQHeadTracker [--host 127.0.0.1] [--port 19765]

            Streams CMHeadphoneMotionManager quaternion packets over UDP to LocusQ HeadTrackingBridge.
            """
        }
    }
}

func run() -> Int32 {
    do {
        let options = try CliOptions.parse(CommandLine.arguments)
        let sender = try UdpSender(host: options.host, port: options.port)
        let app = TrackerApp(udpSender: sender)

        try app.start()
        print("[LocusQHeadTracker] streaming to \(options.host):\(options.port)")
        print("[LocusQHeadTracker] press Ctrl+C to stop")
        RunLoop.main.run()
        return 0
    } catch CliError.help {
        print(CliError.help.description)
        return 0
    } catch {
        print("[LocusQHeadTracker] \(error)")
        print(CliError.help.description)
        return 1
    }
}

exit(run())
