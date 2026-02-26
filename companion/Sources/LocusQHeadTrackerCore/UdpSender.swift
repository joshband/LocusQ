import Foundation

#if canImport(Darwin)
import Darwin
#elseif canImport(Glibc)
import Glibc
#endif

public final class UdpSender: @unchecked Sendable {
    private let socketFd: Int32
    private var destination: sockaddr_in

    public init(host: String = "127.0.0.1", port: UInt16 = 19765) throws {
        #if canImport(Darwin)
        let socketType = SOCK_DGRAM
        #else
        let socketType = Int32(SOCK_DGRAM.rawValue)
        #endif

        socketFd = socket(AF_INET, socketType, 0)
        guard socketFd >= 0 else {
            throw UdpSenderError.socketCreateFailed(errno)
        }

        var addr = sockaddr_in()
        addr.sin_family = sa_family_t(AF_INET)
        addr.sin_port = port.bigEndian

        let convertResult = host.withCString { cs in
            inet_pton(AF_INET, cs, &addr.sin_addr)
        }
        guard convertResult == 1 else {
            close(socketFd)
            throw UdpSenderError.invalidHost(host)
        }

        destination = addr
    }

    deinit {
        close(socketFd)
    }

    public func send(_ payload: Data) throws {
        let sent = payload.withUnsafeBytes { bytes in
            var addr = destination
            return withUnsafePointer(to: &addr) { ptr -> ssize_t in
                ptr.withMemoryRebound(to: sockaddr.self, capacity: 1) { sockPtr in
                    sendto(socketFd, bytes.baseAddress, payload.count, 0, sockPtr, socklen_t(MemoryLayout<sockaddr_in>.size))
                }
            }
        }

        guard sent == payload.count else {
            throw UdpSenderError.sendFailed(errno)
        }
    }
}

public enum UdpSenderError: Error, CustomStringConvertible {
    case socketCreateFailed(Int32)
    case invalidHost(String)
    case sendFailed(Int32)

    public var description: String {
        switch self {
        case .socketCreateFailed(let e):
            return "socket creation failed (errno=\(e))"
        case .invalidHost(let h):
            return "invalid host: \(h)"
        case .sendFailed(let e):
            return "UDP send failed (errno=\(e))"
        }
    }
}
