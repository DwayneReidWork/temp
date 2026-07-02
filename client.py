import re
import socket
import struct
import sys

ADD_STUDENT = 1
SEARCH_STUDENT = 2
SAVE = 3
DISCONNECT = 4

OPTIONS = [ADD_STUDENT, SEARCH_STUDENT, SAVE, DISCONNECT]
HEADER = "!BI"

# Custom exceptions (inheriting from the general exception class)
# class ClientSendException(Exception):
#     def __init__(self, message):
#         super().__init__(message)

class Student:
    def __init__(self, student_id, name, major):
        self.student_id = student_id
        self.name = name
        self.major = major

def main():
    host = "127.0.0.1"
    port = 1234
    head_size = struct.calcsize(HEADER)
    print(f"Header size {head_size}")
    # error handle if connect fails
    try:
        c = connect(host,port)
    except socket.error:
        print("Error connecting")

    #loop for the entire program
    while (True):
        user_interactive_bit(c)
        

def connect(host, port):
    # socket.create_connection as an alternative
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host,port))
    print("Connected to server.")
    #Return socket to close later?
    return s

#Safe Send Loop
#returns bool
def sendall(socket, data) -> bool:
    sent_bytes = 0
    total_data = len(data)

    while (sent_bytes < total_data):
        try:
            #If all bytes dont send. index into data total number of bytes sent and try to send rest send rest 
            sent_bytes += socket.send(data[sent_bytes:])
        except ConnectionError:
        # except socket.error:
            print("Send error.\n")
            return False
    return True

#safe recv
def recv_expected(socket, expected_len) -> bytes:
    recvd_bytes = b""
    while (len(recvd_bytes) < expected_len):
        chunk = socket.recv(expected_len - len(recvd_bytes))
        if not chunk:
            raise ConnectionError("Socket closed before receiving expected data")
            #raise SocketError     
        recvd_bytes += chunk   
    return recvd_bytes

def get_reply(socket):
    print("Inside get reply")
    packet_type = recv_expected(socket, 1)
    # should be a single byte
    #Recieved packet type: b'\x03'
    #can't do packet_type.decode()?
    #recv above gets it as a tuple!!!!!!!!!!! Need to index it into a number!!!!
    packet_type = packet_type[0]
    
    if packet_type not in OPTIONS:
        print("Invalid packet recieved")
    print(f"Recieved packet type: {packet_type}")
    
    match packet_type:
        case 1: 
            print("add response reply")
            buff = recv_expected(socket, 4)
            msg_len = struct.unpack("!I", buff)[0]
            msg = recv_expected(socket, msg_len).decode()
            print(msg)
            pass
        case 2:
            print("search response reply")
            buff = recv_expected(socket, 4)
            msg_len = struct.unpack("!I", buff)[0]
            msg = recv_expected(socket, msg_len).decode()
            print(msg)

            #psuedocode
            #data should be formatted I I ?S I ?S
            # id, name len, name, major len, major 
            ##end problem area
        case 3:
            print("save response reply")
            buff = recv_expected(socket, 4)
            msg_len = struct.unpack("!I", buff)[0]
            msg = recv_expected(socket, msg_len).decode()
            print(msg)
        case 4:
            print("Closing socket...\n")
            socket.close()
        case _:
            pass

#Passs in the socket to send data. Can probably fix that somehow
def user_interactive_bit(c=socket):
    #loop specifically for the question porompt
    while (True):
        try:
            uInput = int(input("Select what would you like to do.\n1. Add Student\n2. Search For a Student\n3. Save\n4. Disconnect\n->"))
            if uInput not in OPTIONS:
                print("Invalid option please select 1,2,3 or 4.\n")
                continue
            break
        except ValueError:
            print("Please select a number 1-4.\n")

    match uInput:
        case 1:
            while (True):
                try:
                    #Regex for 1 or 2 words on either side of a colon <fname lname:2 word major>, or just 1 name and 1 word major
                    #/w+ = word (? \w+)? = optional word)
                    pattern = r"(\w+(?: \w+)?):(\w+(?: \w+)?)"
                    uInput = input("Name:major: ")
                    match = re.fullmatch(pattern, uInput)
                    if match:
                        name = match.group(1).encode()
                        major = match.group(2).encode()
                        #dont forget the '!' in the add format string because byte ordermatters here too not  just the header
                        #but also not can't just do HERADER+add_fmt_string because of the exclamation point in the middle
                        #can do it with indexing. to cut off first char '!" in add fmt string"
                        #indexed_string = f"{HEADER}{payload_fmt[1:]}"
                        #alternatively can -> full_fmt = HEADER + payload_fmt.lstrip("!")

                        payload_fmt = f"!I{len(name)}sI{len(major)}s"
                        full_fmt = f"!BII{len(name)}sI{len(major)}s"
                        payload_size = struct.calcsize(payload_fmt)

                        add_packet = struct.pack(full_fmt, ADD_STUDENT, payload_size, len(name), name, len(major), major)
                        
                        print(add_packet)
                        sendall(c, add_packet)
                        print("Data Sent.")
                        ##recv data back
                        get_reply(c)
                        break
                    print("Please try again.")
                except ValueError:
                    print("Please enter a name and major in the following format\nName:major.\n")
        case 2:
            while (True):
                try:
                    uInput = input("Please enter a 4 digit ID to search for: ")
                    if not (uInput.isdigit() and len(str(uInput)) == 4):
                        print("Invalid option please enter a number between 0000 and 9999\n")
                        continue
                    break
                except ValueError:
                    print("Please enter a number between 0000 and 9999.\n")

            uInput = int(uInput)
            search_packet = struct.pack(f"{HEADER}I", SEARCH_STUDENT, len(str(uInput)),  uInput)
            print(search_packet)
            sendall(c, search_packet)
            print("Data Sent.")
            ##recv data back
            get_reply(c)

        case 3:
            save_packet = struct.pack(HEADER, SAVE, 0)
            print(save_packet)
            sendall(c, save_packet)
            get_reply(c)
        case 4:
            disconnect_packet = struct.pack(HEADER, DISCONNECT, 0)
            print(disconnect_packet)
            sendall(c, disconnect_packet)
            get_reply(c)
            sys.exit()
        case _:
            print("How'd did we get here.....? Nobody posed to be here")

if __name__ == "__main__":
    main()
