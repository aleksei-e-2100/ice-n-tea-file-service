<p align="right">
  <a href="./README.md">Русский</a> | <strong>English</strong> 
</p>

# iCE && tEA File Service

A small but full-featured file transfer service over a network, implemented in C++ and Linux.

The project uses its own application-level protocol over TCP, supports IPv4 and IPv6, node authentication using keys, encryption of transmitted data, and processing of a large number of connections within a single thread using epoll.

## Annotation

iCE && tEA File Service is a client-server application for secure file transfer between preconfigured network nodes.

TCP serves as the transport layer foundation, providing reliable data delivery. On top of TCP, a custom message exchange protocol is implemented, including a handshake, mutual authentication, shared secret generation, and file transfer with integrity verification.

The application is intended for Linux and is delivered as a single executable file that can run both in server mode and client mode.


## Key Features

### Reliable Data Transfer

File transfer is performed over TCP. As long as the connection is not broken, data will be delivered without loss and in the correct order.

### Security

Each node has its own key pair.

Before any transfer begins, both parties perform mutual cryptographic verification and establish a shared session secret. After that, all application traffic is encrypted.

### Key Generation

The application can independently generate cryptographic key pairs used for node authentication and secure connection establishment. 

This allows quick configuration setup without external tools.

### No "Trust On First Use" Model

Unlike SSH with its Trust On First Use model, public keys of known nodes are specified in advance in the configuration. 

This makes it possible to detect node substitution attempts already during the first connection.

### Working with Server Names

The user does not need to remember IP addresses of remote nodes.

Sending (receiving) is performed using the server name specified in the configuration file.

Example:

```
filesvc --send office-server report.pdf
```
### IPv4 and IPv6 Support

Both address families are supported.

### NAT Compatibility

The client may be located behind NAT (home router, corporate network, etc.).

A public IP address is required only for the server.

### Free Space Verification

Before receiving a file, the service (both in server mode and client mode) checks available disk space and can reject the request before the transfer starts.

### Safe File Saving

A received file is never written directly under its final name. 

Data is first saved into a temporary file. Only after successful completion of the transfer is it renamed to the final file.

This helps avoid corruption or overwriting of existing data in case of connection loss, transfer errors, or insufficient disk space.

### Large File Transfer

The file is never loaded fully into memory.

Transfer is performed sequentially in fixed-size blocks, allowing files of any size to be processed.

For network buffers, a custom container based on std::vector without automatic memory initialization is used. This avoids unnecessary zeroing and copying operations when working with large data volumes.

Data is read directly from disk into the network buffer, then encrypted and immediately sent over the network. This minimizes intermediate memory operations and reduces CPU load.

### Efficient Network Utilization

Transfer is not performed by sending a single block and waiting for its confirmation. The channel maintains a transfer window containing multiple confirmed blocks simultaneously.

As confirmations arrive, the window moves forward and frees space for new blocks.

This approach allows better utilization of available bandwidth and reduces the impact of latency on transfer speed.

### Detailed Error Diagnostics

Most components return a detailed textual description of the last error, simplifying troubleshooting and debugging.

### Server Flexibility

The server can be launched as a regular application or run in the background under systemd.

This makes it possible to use it as a full system service with automatic startup after reboot, centralized management, and built-in Linux monitoring facilities.


## Architecture

The project is built around the principle of separation of responsibilities.

Responsibilities are divided between connection management, application protocol implementation, and data transfer operations. 

Each layer is responsible only for its own task, simplifying maintenance and allowing independent development of different parts of the system.

### Managers

Responsible for:
- TCP connection creation;
- epoll handling;
- system event processing;
- creation and removal of transfer channels.

### Channels

Encapsulate the application protocol logic.

The channel determines what messages should be sent, what should be expected from the remote side, and how the session should progress.

### Executor Classes

Separated into independent components:
- handshake (Handshake);
- task negotiation (Task);
- file sending (FileSend);
- file receiving (FileReceive).

This approach allows protocol development without turning a single class into a "god object".

### Session Context

Shared session data is stored in a separate SessionCtx object.

The context contains the current transfer state, file parameters, shared encryption secret, network buffers, and auxiliary information.

All components involved in connection processing work through the same context, avoiding data duplication and reducing coupling between classes.

### Finite State Machine

Each transfer channel is represented as a finite state machine that sequentially moves the connection through protocol states: handshake, task negotiation, data transfer, session termination.

Transition to the next state occurs only after successful completion of the previous stage or after receiving the required data from the remote side.

Advantages of this approach:
- protocol logic is explicitly described;
- no hidden transitions between stages;
- convenient maintenance and extension;
- easy integration with a non-blocking execution model.

### Why epoll

The application is built according to the Event-Driven Architecture (Reactor Pattern).

epoll is used for handling network events.

This provides several advantages:
- one thread can serve many clients;
- no thread context-switching overhead;
- minimal resource consumption while idle;
- good scalability with many connections.

Despite the absence of multiple worker threads, the server is capable of serving many clients simultaneously.

This approach can be considered emulated multithreading implemented through non-blocking event processing.

## Communication Protocol

File transfer consists of several stages.

### Handshake

Client:
- sends its public key;
- sends an ephemeral session key;
- signs the message.

Server:
- verifies the client against the configuration;
- verifies the signature;
- sends its own ephemeral key;
- signs the response.

### Secret Generation

After the handshake both sides independently compute the same shared secret. The secret is never transmitted over the network.

### Task Negotiation

The client sends a request:
- send a file;
- receive a file.

The server:
- validates parameters;
- checks file existence;
- checks available disk space;
- accepts or rejects the request.

### Data Transfer

After negotiation, file block transfer begins.

Each block:
- is encrypted;
- includes an authentication tag;
- is confirmed by the recipient.

### Integrity Verification

Every message undergoes cryptographic integrity verification.

If data was corrupted or modified during transfer, the message will be rejected.

To protect against message replay and ensure correct encryption operation, each side maintains two independent nonce counters: incoming and outgoing.

Nonce values are not transmitted over the network and do not increase packet size. They are calculated locally in the same order on both sides of the connection. This approach is possible because TCP guarantees preservation of data delivery order.


## Configuration

Configuration is stored in file `filesvc.conf`.

The file must be located in the same directory as the binary executable or, when using systemd, in the directory specified by the `WorkingDirectory` parameter in `filesvc.service`.

#### The file contains:
- settings of the local node;
- settings of other known nodes;
- public keys of remote nodes;
- network connection parameters.

#### Features
- duplicate sections are prohibited;
- duplicate parameter definitions are prohibited;
- unknown lines are considered errors;
- local node configuration is mandatory;
- at least one remote node must exist;
- depending on the number of specified parameters, a remote node may be configured either only as a client (requests can only be accepted from it) or as a full server (requests can be sent to it).

#### To configure the local node, the following is required:
- create a `[~]` section;
- specify the local ED25519 private key in it;
- specify an existing directory (files requested from servers will be stored there, and files requested by connecting clients will be sent from there if the local node acts as a server).

For the local node to operate as a server, its IP address and port on which it accepts connections must also be specified.

#### To configure a remote node, the following is required:
- create a section named after the remote node, for example `[office_server]`;
- specify the remote node's ED25519 public key in it.

For the remote node to operate as a server, its IP address and port must also be specified.

#### Параметры конфигурации

- `[~]` - self (local) section (all key-value pairs below belong to it until the next section);
- `[remote_node_name]` - remote node section (all key-value pairs below belong to it until the next section);
- `private_key: key` - private key (local section only). Value: Base64 string without quotes;
- `public_key: key` - public key (remote node section only). Value: Base64 string without quotes;
- `dir: directory` - working directory (local section only). Files received from remote servers are stored there. Files requested by remote nodes are transferred from there. Value: existing directory;
- `ip: IPv4 or IPv6 address` - required if the node is configured to operate as a server;
- `port: port` - required if the node is configured to operate as a server.

A sample configuration file is provided in the `release` directory.

### Configuration Subsystem Architecture

The configuration subsystem consists of several independent layers.

First, the file is parsed into a set of raw records. Then the validator performs checks of the configuration structure, parameters, and dependencies between sections. After that, the result becomes available through a unified access interface.

This separation allows parsing and validation stages to evolve independently.

For example, if needed, the configuration file can be migrated to JSON or any other format without changing validation logic. It is sufficient to implement a new parser producing the same intermediate data representation.

### Configuration Checking

Before using the application, configuration can be checked:

```
filesvc --health
```

The result will show whether the application is ready for operation and provide a detailed list of detected errors.


## Build

### Requirements

- Linux
- GCC
- CMake
- OpenSSL Development Package

For Debian/Ubuntu:

```
sudo apt install build-essential cmake libssl-dev
```

The project uses static OpenSSL linking. After build completion, OpenSSL does not need to be installed on the target system.

### Instructions

Regular CMake build:

```
git clone <repository> 
cd <repository> 

mkdir build 
cd build 

cmake -DCMAKE_BUILD_TYPE=Release .. 
cmake --build .
```

Executable file:

```
release/filesvc
```

Configuration example:

```
release/filesvc.conf
```

Example systemd .service file

```
release/filesvc.service
```


## Usage

The application is a single executable file capable of running both as a server and as a client. Before use, keys must be generated, the configuration file must be configured, and configuration validation must complete successfully.


### Main Commands

- Generate an ED25519 cryptographic key pair required for node authentication and secure connections:

    ```
    filesvc --keygen
    ```

- Validate configuration and display detailed information about all discovered errors:

    ```
    filesvc --health
    ```

- Run in server mode and wait for incoming connections from known clients listed in the configuration:

    ```
    filesvc --listen
    ```

- Send a file to a remote server. The `server_name` parameter specifies the name of the remote host as defined in the configuration file, while `file_path` can be either an absolute or a relative path to the file on the local machine.

    ```
    filesvc --send <server_name> <file_path>
    ```

    Example:

    ```
    filesvc --send office-server report.pdf
    ```

- Receive a file from a remote server. The `file_name` parameter specifies the name of a file located in the server's working directory:

    ```
    filesvc --recv office-server report.pdf
    ```

    Example:

    ```
    filesvc --recv office-server report.pdf
    ```

- To get complete help on all supported commands and parameters, run `filesvc` without arguments.


## Running the Server as a System Service (systemd)

Recommended for background operation:

- Make sure systemd is running on your Linux server.

- Edit `filesvc.service` (located in the `release` directory) according to your settings.

- Copy it to the systemd directory: 

    ```
    sudo cp filesvc.service /etc/systemd/system/
    ```

- Reload systemd configuration: 

    ```
    sudo systemctl daemon-reload
    ```

- Start the server: 

    ```
    sudo systemctl start filesvc.service
    ```

- Check server status: 

    ```
    sudo systemctl status filesvc.service
    ```

- Enable automatic startup (so that it launches when the system starts): 

    ```
    sudo systemctl enable filesvc.service
    ```

Server management through systemd:

- Stop the server: 

    ```
    sudo systemctl stop filesvc.service
    ```

- Disable automatic startup: 

    ```
    sudo systemctl disable filesvc.service
    ```


## Technology Stack
- C++
- Linux
- TCP
- IPv4 / IPv6
- epoll
- OpenSSL
- ED25519
- X25519
- ChaCha20-Poly1305
- Event-Driven Architecture
- Finite State Machine
- Shared Session Context
- Single Binary Deployment
- Single Thread Runtime


## Limitation

- как клиенту, так и серверу задается строго определенная директория для работы. Запросить файл можно только из нее.  Передаваемый файл тоже будет записываться в такую директорию. Такое ограничение вызвано целью создания данного проекта. При этом возможность клиента передать на сервер абслюьно любой файл из своей ОС, к которому у него есть права доступа, сохраняется.

Both client and server operate within a strictly defined working directory. A file can only be requested from that directory. Received files are also written into such a directory. This limitation is dictated by the purpose of this project. At the same time, the client retains the ability to send absolutely any file from its operating system to which it has access rights.


## TODO
- wildcard requests for sending (receiving) multiple files at once;
- ability to write files not only into the server (client) working directory, but into any directory;
- client command for viewing the contents of the server's working directory.
