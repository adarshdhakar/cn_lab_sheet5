# Chat Server and Client using TCP Sockets 

## Table of Contents

- [Introduction](#introduction)
- [Group Details](#group-details)
- [Required Links](#required-links)
- [File Structure](#file-structure)
- [Compilation & Execution](#compilation--execution)
- [Features](#features)
- [Additional Features](#additional-features)
- [Images](#images)
- [Submission](#submission)

## Introduction
This project implements a multi-client chat server using C++ and TCP sockets, with the C code provided as reference. The server is designed to handle multiple clients simultaneously using threads and the `select()` system call.

## Group Details

| Name              | Roll No      |
|------------------|-------------|
| Adarsh Dhakar    | 22CS01040    |
| Avik Sarkar      | 22CS01060    |
| Debargha Nath    | 22CS01070    |
| Soham Chakraborty| 22CS02002    |

## Required Links
- [Github Repository](https://github.com/adarshdhakar/cn_lab_sheet5)
- [Download Zip File](CN_LAB_SHEET5.zip)
- [Report](Report.pdf)
- [Images Folder](images/)

## File Structure
- `server_threaded.cpp` - Multi-client chat server using threads.
- `server_select.cpp` - Multi-client chat server using the `select()` system call.
- `client.cpp` - Chat client program.
- `README.md` - This file, containing details of the project.
- `images/` - Screenshots of the running server and clients on separate hosts.
- `report.pdf` - Contains group details, explanations, and additional functionalities.

## Compilation & Execution
The below procedure can be followed after:
- Cloning the repository: 
  ```bash
  git clone https://github.com/adarshdhakar/cn_lab_sheet5 
  cd cn_lab_sheet5
  ```
- Downloading and extracting the zip file.
- Or just downloading the server and client files.

### Compiling the Server and Client
```bash
# Compile server using threads
g++ server_threaded.cpp -o server_thread -lpthread

# Compile server using select()
g++ server_select.cpp -o server_select

# Compile client
g++ client.cpp -o client
```

### Running the Server
```bash
./server_thread [PORT]
```
or
```bash
./server_select [PORT]
```

### Running the Client
```bash
./client [SERVER_IP] [PORT]
```

## Features
- Multiple clients can connect to the server simultaneously.
- Clients can send private messages to each other via the server.
- Clients can broadcast messages to all connected clients.
- Clients can join or disconnect from the chat without affecting others.
- Server handles client timeout and disconnections.

## Additional Features
- Timeout feature to detect inactive clients.

## Images
- **Server**  
<table>
    <tr>
    <td><img src="images/1.png" alt="Image 1" width="500"></td>
    <td><img src="images/2.png" alt="Image 2" width="500"></td>
    </tr>
</table>

- **Client1**  
<table>
    <tr>
    <td><img src="images/3.png" alt="Image 3" width="500"></td>
    <td><img src="images/4.png" alt="Image 4" width="500"></td>
    </tr>
</table>

- **Client2**  
<table>
    <tr>
    <td><img src="images/5.png" alt="Image 5" width="500"></td>
    <td><img src="images/6.png" alt="Image 6" width="500"></td>
    </tr>
</table>

- **Client3** 
<table>
    <tr>
    <td><img src="images/7.png" alt="Image 7" width="500"></td>
    <td><img src="images/8.png" alt="Image 8" width="500"></td>
    </tr>
</table>

## Submission
- All codes are included in a zip file.
- Report with explanations and images is attached.
- Github link for the same is provided.

