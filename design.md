# a-chat design document

a secure, end-to-end encrypted group messaging service

## overview

 - **purpose**: a-chat is a group messaging application that allows users to send and receive end-to-end encrypted messages
 - **scope**: users can host or join a secure session, and communicate using a tcp connection
 - **technologies**: c99, aes encryption (AES-256), posix sockets, and posix threads (pthreads)

## requirements

### core features

 - create a complete, stable, and reliable tcp connection with no critical bugs
 - reliably sending and receiving messages in real time
 - encrypting outgoing messages and decrypting incoming messages reliably
 - no authentication/account is needed, only a username and the server's ip address and port

## architecture

### server

 - accepts multiple client connections through the use of client handlers
 - forwards encrypted messages to all connected clients
 - does **NOT** store or decrypt any messages (zero-knowledge)

### client

 - connects to the server over tcp
 - encrypts messages locally before being sent to the server
 - decrypts messages upon arrival
 - handles user input/output

### encryption

 - todo

### message flow

 - client establishes a tcp connection with the server
 - key exchange happens between clients
 - messages are encrypted by the client, sent to the server, then broadcasted to the other clients
 - receiving clients decrypt the message

### threading model

 - one thread per client connection on the server
 - client uses two threads for sending and receiving
