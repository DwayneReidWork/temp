#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*For inet_ntoa*/
#include <arpa/inet.h>

#define MAX_NAME_LENGTH 255
#define MAX_STUDENTS 100
#define PORT 1234
#define ERROR -1
#define SUCCESS 0

char *recv_expected(int sock, size_t expected_len);
int sendall(int sock, char *data, ssize_t data_len);
void handle_client(int client_socket);
void variable_frees(int count, ...);

/// @brief Packet Types, Packed for alignment
typedef enum __attribute__((__packed__))
{
    ADD_STUDENT = 1,
    SEARCH_STUDENT,
    SAVE,
    DISCONNECT,
} PacketType;

/// @brief Student struct containing int id and char* name/major
typedef struct Student
{
    int id;
    char *name;
    char *major;
} Student_t;

/// @brief Struct for packet headers containing PacketType and uint32_t payload
typedef struct Header
{
    PacketType p_type;
    uint32_t payload_len;
} __attribute__((packed)) Header_t;

/// @brief Database struct containing pointer to Student_t objects and count of students
typedef struct Database
{
    Student_t *Student[MAX_STUDENTS];
    int num_students;
} Db_t;

// Empty database.
Db_t database = {0};

/// @brief Creates a student_t object
/// @param id Student ID number (generated)
/// @param name Student name
/// @param major Student major
/// @return Student_t object on success, else NULL
Student_t *create_student(int id, const char *name, const char *major)
{
    // Null check passed in ptrs!
    if (NULL == name)
    {
        perror("Name Null");
        return NULL;
    }
    if (NULL == major)
    {
        perror("Major Null");
        return NULL;
    }

    Student_t *s = calloc(sizeof(Student_t), 1);
    if (NULL == s)
    {
        printf("Calloc Failed.\n");
        return NULL;
    }

    s->id = id;

    s->name = calloc(strlen(name) + 1, 1);
    if (NULL == s->name)
    {
        printf("Calloc Failed.\n");
        // free student because it was allocated
        free(s);
        return NULL;
    }
    strcpy(s->name, name);

    s->major = calloc(strlen(major) + 1, 1);
    if (NULL == s->major)
    {
        printf("Calloc Failed.\n");
        free(s->name);
        free(s);
        return NULL;
    }
    strcpy(s->major, major);

    return s;
}

/// @brief search for student by Id
/// @param id identifier for each student
/// @return student data if found, else NULL
Student_t *find_student(int id)
{
    if (0 == database.num_students)
    {
        printf("Database Empty.\n");
        return NULL;
    }

    for (int i = 0; i < database.num_students; i++)
    {
        if (database.Student[i]->id == id) // check if null before deref
        {
            return database.Student[i];
        }
    }
    return NULL;
}

/*
// NO LONGER NEEDED SINCE READING FROM FILE.
/// @brief create student database
void database_setup()
{
    // currently manually generating database every time.
    // Read from file!!!
    database.num_students = 0;
    database.Student[database.num_students++] = create_student(1001, "Alice", "CS");
    database.Student[database.num_students++] = create_student(1002, "Bob", "Math");
    database.Student[database.num_students++] = create_student(1003, "Carol", "Physics");
}
*/

/// @brief free student
/// @param student Pointer to struct of student name and major and id (id not dynamically allocated so not freed)
void free_student(Student_t *student)
{
    if (NULL == student)
    {
        return;
    }

    free(student->name);
    free(student->major);
    free(student);
}

/// @brief Frees variable number of buffers passed in
/// @param count number of items passed in
/// @param variable number of char*'s
void variable_frees(int count, ...)
{
    // c func doesn't know how many args were passed. Need to pass count
    va_list args;
    va_start(args, count);
    // loop through args
    for (int i = 0; i < count; i++)
    {
        /*set each arg to a char* because its all buffers
        Use void ptr to be generic?*/
        char *ptr = va_arg(args, char *);
        if (NULL != ptr)
        {
            // free the ptrs
            free(ptr);
        }
    }
    va_end(args);
}

/// @brief Frees databasee of students and sets count to 0
void database_cleanup()
{
    for (int i = 0; i < database.num_students; i++)
    {
        free_student(database.Student[i]);
        database.Student[i] = NULL;
    }
    // Don't forget to reset num students to 0
    database.num_students = 0;
}

void print_database(void)
{
    printf("Student Database\n");
    printf("-----------------------------\n");

    for (int i = 0; i < database.num_students; i++)
    {
        Student_t *student = database.Student[i];

        printf("ID: %d\n", student->id);
        printf("Name: %s\n", student->name);
        printf("Major: %s\n", student->major);
        printf("-----------------------------\n");
    }
}

int save_to_file(Db_t *database)
{
    if (NULL == database)
    {
        printf("Null database. Nothing to save.\n");
        return ERROR;
    }

    FILE *save_file = fopen("./students.txt", "wb");
    if (NULL == save_file)
    {
        printf("Error opening file.\n");
        return ERROR;
    }

    // Save number of students
    fwrite(&database->num_students, sizeof(database->num_students), 1, save_file);

    for (int i = 0; i < database->num_students; i++)
    {
        // each student object
        Student_t *s = database->Student[i];

        // name and major lenths
        uint32_t name_len = strlen(s->name);
        uint32_t major_len = strlen(s->major);

        // write the ID as a just an int
        fwrite(&s->id, sizeof(int), 1, save_file);

        // name len then name
        // TODO Clarification

        // fwrite(ptr_to_data, size_of_1_element, num_of_elements, output_file)
        fwrite(&name_len, sizeof(uint32_t), 1, save_file);
        // fwrite(ptr_to_data, 1_for_char? [use sizeof(char?)], names_len_of_chars?, output_file)
        // fwrite(s->name, 1, name_len, save_file);
        fwrite(s->name, sizeof(char), name_len, save_file);

        // same for major?
        //  major len then major
        fwrite(&major_len, sizeof(uint32_t), 1, save_file);
        // fwrite(s->major, 1, major_len, save_file);
        fwrite(s->major, sizeof(char), major_len, save_file);
    }

    fclose(save_file);
    return SUCCESS;
}

int read_from_file()
{
    // TODO finish this
    FILE *file = fopen("/home/dreid/HARVARD/STUDENT_DB/output/students.txt", "rb");
    if (NULL == file)
    {
        printf("Error opening file.\n");
        return ERROR;
    }

    int num_students = 0;
    size_t name_len = 0, major_len = 0;

    // Check returns of fread
    // fread returns number of items read/written so check it against number expected (1 for ints or name_len/major_len for the name/major)

    // can I fread directlly into database.numstudents?
    if (1 != fread(&num_students, sizeof(int), 1, file))
    {
        printf("Error reading number of students.\n");
        fclose(file);
        return ERROR;
    }
    database.num_students = num_students;

    for (int i = 0; i < database.num_students; i++)
    {
        Student_t *s = calloc(sizeof(Student_t), 1);
        if (NULL == s)
        {
            printf("Error allocating space for db.\n");
            fclose(file);
            return ERROR;
        }

        if (1 != fread(&s->id, sizeof(int), 1, file))
        {
            printf("Error reading ID number.\n");
            free(s);
            fclose(file);
            return ERROR;
        }

        if (1 != fread(&name_len, sizeof(int), 1, file))
        {
            printf("Error reading name length.\n");
            free(s);
            fclose(file);
            return ERROR;
        }

        s->name = calloc(name_len + 1, 1); // null terminator?
        if (NULL == s->name)
        {
            printf("Error allocating space for name.\n");
            free(s);
            fclose(file);
            return ERROR;
        }
        if (name_len != fread(s->name, sizeof(char), name_len, file))
        {
            printf("Error writing into name.\n");
            free(s);
            fclose(file);
            return ERROR;
        }
        s->name[name_len] = '\0';

        if (1 != fread(&major_len, sizeof(int), 1, file))
        {
            printf("Error reading major length.\n");
            free(s->name);
            free(s);
            fclose(file);
            return ERROR;
        }

        s->major = calloc(major_len + 1, 1); // null terminator?
        if (NULL == s->major)
        {
            printf("Error allocating space for major.\n");
            free(s->name);
            free(s);
            fclose(file);
            return ERROR;
        }
        if (major_len != fread(s->major, sizeof(char), major_len, file))
        {
            printf("Error writing into major.\n");
            free(s->major);
            free(s->name);
            free(s);
            fclose(file);
            return ERROR;
        }
        s->major[major_len] = '\0';

        database.Student[i] = s;
        // feels like a lot of error checking between fread and calloc...
    }
    fclose(file);
    return SUCCESS;
}

/// @brief Set up server connection
/// @return -1 on error, 0 on success.
int connection_setup()
{
    int server_socket = 0, client_socket = 0, reuse = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in); // use sizeof on the type

    server_socket = (socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (-1 == server_socket)
    {
        printf("Socket creation failed\n");
        return ERROR;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (-1 == setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_socket);
        return ERROR;
    }

    if (-1 == bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)))
    {
        printf("Bind failed\n");
        close(server_socket);
        return ERROR;
    }

    // Listen for connections
    if (-1 == listen(server_socket, 5))
    {
        printf("Listen failed\n");
        close(server_socket);
        return SUCCESS;
    }

    printf("Server listening on port %d\n", PORT);

    // Accept and handle clients
    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (-1 == client_socket)
        {
            perror("Client connection failed");
            continue;
        }

        printf("Connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_socket);

        close(client_socket);
    }
    return SUCCESS;
}

/// @brief  Parses messages revievd sorting by header and interpreting data based on message type
/// @param client_socket
void handle_client(int client_socket)
{
    while (1)
    {
        /*
        Work for sending back requested data.
        Working with assumption header is just packet type then length
        cast recv expected to header_t ptr type
        */

        // THIS IS MY PROBLEM AREA
        Header_t header;
        char *header_buff = recv_expected(client_socket, sizeof(Header_t));
        if (NULL == header_buff)
        {
            return;
        }
        memcpy(&header.p_type, header_buff, sizeof(header.p_type));
        printf("Packet Type: %u\n", header.p_type);
        // FIX THE OFFSET (2ND VARIABLE)
        memcpy(&header.payload_len, header_buff + sizeof(header.p_type), sizeof(header.payload_len));
        header.payload_len = ntohl(header.payload_len);
        printf("Payload Len: %u\n", header.payload_len);
        free(header_buff);
        // can't do this (below) since i'm only recving 1 byte and then switching on that move below printf into the specific switch case!!!!!!!
        // printf("Payload (Network order): %u\n", header.payload);
        //  END OF PROBLEM AREA
        int response_len = 0, space_needed = 0, space_calculated = 0;

        switch (header.p_type)
        {
        case 1:
            /*
            Issue with current packet setup. When recieving multi-part data like name_len then name and major_len then major that first num gets stored as the payload in the header which is managable but only if I know thats what that first bit of data is like for case 1 that number is the len of the name but for case 2 thats the entire student ID and case 3 its just "0000" because disconnect has no real 'payload'.
            */
            printf("Inside case 1.\n");
            char *data = recv_expected(client_socket, header.payload_len);
            if (NULL == data)
            {
                return;
            }

            // To move through buffer
            size_t offset = 0;

            // Name length
            uint32_t name_len;
            memcpy(&name_len, data + offset, sizeof(uint32_t));
            name_len = ntohl(name_len);
            // update offet
            offset += sizeof(uint32_t);

            // Name
            char *name = calloc(name_len + 1, 1);
            if (NULL == name)
            {
                free(data);
                return;
            }
            memcpy(name, data + offset, name_len);
            name[name_len] = '\0';
            // update offset
            offset += name_len;

            // Major length
            uint32_t major_len;
            memcpy(&major_len, data + offset, sizeof(uint32_t));
            major_len = ntohl(major_len);
            // update offset
            offset += sizeof(uint32_t);

            // Major
            char *major = calloc(major_len + 1, 1);
            if (NULL == major)
            {
                variable_frees(2, data, name);
                return;
            }
            memcpy(major, data + offset, major_len);
            major[major_len] = '\0';
            // update offset
            offset += major_len;

            printf("Name: %s\n", name);
            printf("Major: %s\n", major);

            int student_ID = 1001 + database.num_students;
            database.Student[database.num_students++] =
                create_student(student_ID, name, major);

            variable_frees(3, data, name, major);

            print_database();

            // TODO Edit this to send back name and major added and the ID number
            char *mesg = "Recieved data to add";
            ssize_t msg_len = strlen(mesg);
            Header_t add_reply = {0};
            add_reply.p_type = ADD_STUDENT;
            add_reply.payload_len = htonl(msg_len);
            sendall(client_socket, (char *)&add_reply, sizeof(Header_t));
            sendall(client_socket, mesg, msg_len);
            printf("Add reply sent.\n");
            /*
            cleanup function for all the frees? Each set of frees is 1 more longer.
            void cleanup(char*, ...) SOLVED made variable free func*/
            break;

        case 2:
            printf("Inside case 2.\n");
            data = recv_expected(client_socket, header.payload_len);
            if (NULL == data)
            {
                return;
            }
            uint32_t s_ID = 0;
            memcpy(&s_ID, data, sizeof(uint32_t));
            s_ID = ntohl(s_ID);
            Student_t *s = find_student(s_ID);
            char *msg = NULL;

            // Prints to become send packets as replies to client
            if (s != NULL)
            {
                // PROBLEM AREA
                // snprintf returns num of bytes written
                // can also manually calculate the space needed.
                space_needed = snprintf(NULL, 0, "FOUND:%d,%s,%s",
                                        s->id,
                                        s->name,
                                        s->major);

                // 8 = "FOUND:" + the two commas between the data
                space_calculated = 8 + sizeof(int) + strlen(s->name) + strlen(s->major);
                printf("Space Calculated: %d\nSpace needed: %d\n", space_calculated, space_needed);

                msg = calloc(space_needed + 1, 1);

                snprintf(msg, space_needed + 1, "FOUND:%d,%s,%s",
                         s->id,
                         s->name,
                         s->major);
            }
            else
            {
                space_needed = snprintf(NULL, 0, "NOT_FOUND");
                msg = calloc(space_needed + 1, 1);
                snprintf(msg, space_needed + 1, "NOT_FOUND");
            }
            // END PROBLEM AREA!!!
            msg_len = strlen(msg);
            Header_t search_reply = {0};
            search_reply.p_type = SEARCH_STUDENT;
            search_reply.payload_len = htonl(msg_len);

            // send header data
            sendall(client_socket, (char *)&search_reply, sizeof(Header_t));
            // send data
            sendall(client_socket, msg, msg_len);
            printf("Search Response Sent.\n");
            free(data);
            free(msg);
            break;
        case 3:
            printf("Inside save case:\n");
            int status = save_to_file(&database);
            if (-1 == status)
            {
                msg = "Error saving file";
            }
            else
            {
                msg = "Save successful\n";
            }
            msg_len = strlen(msg);
            Header_t save_reply = {0};
            save_reply.p_type = SAVE;
            save_reply.payload_len = htonl(msg_len);

            // send header data
            sendall(client_socket, (char *)&save_reply, sizeof(Header_t));
            // send data
            sendall(client_socket, msg, msg_len);
            printf("Search Response Sent.\n");
            break;
        case 4:
            printf("Inside case 4.\n");
            printf("Shutdown Packet recieved\n");
            Header_t shutdown_reply = {0};
            shutdown_reply.p_type = DISCONNECT;
            response_len = sizeof(shutdown_reply);
            sendall(client_socket, (char *)&shutdown_reply, response_len);
            // Find a more graceful way to kill program.
            database_cleanup();
            close(client_socket);
            exit(EXIT_SUCCESS);
        default:
            printf("Invalid packet type\n");
        }
    }
}

int main()
{
    read_from_file();
    connection_setup();
    database_cleanup();
    return SUCCESS;
}

int sendall(int sock, char *data, ssize_t data_len)
{
    ssize_t sent_bytes = 0;
    while (sent_bytes < data_len)
    {
        // Sent chunk = sent data
        ssize_t sent_chunk = send(sock, data + sent_bytes, data_len - sent_bytes, 0);

        if (sent_chunk == -1) // handle case where sent_bytes is 0
        {
            printf("Send Error\n");
            return -1;
        }
        // Hit every time messages done sending...
        // Fixed Was checking sent bytes not sent chunk
        if (0 == sent_chunk)
        {
            perror("0 bytes sent");
            return ERROR;
        }
        // add chunk to total bits sent to move "ptr"
        sent_bytes += sent_chunk;
    }
    printf("Send Successful.\n");
    return SUCCESS;
}

// returns ptr to data recvd
char *recv_expected(int sock, size_t expected_len)
{
    // buffer for data
    char *recv_buffer = calloc(expected_len + 1, 1);

    if (NULL == recv_buffer)
    {
        printf("Memory Allocation failed.\n");
        return NULL;
    }
    // size_t unsigned | ssize_t signed
    size_t total_recvd = 0;
    while (total_recvd < expected_len)
    {
        // recv(from, into, offset/size, flags)
        ssize_t bytes_recvd = recv(sock,
                                   recv_buffer + total_recvd,
                                   expected_len - total_recvd, 0);

        if (0 >= bytes_recvd)
        {
            free(recv_buffer);
            return NULL;
        }
        total_recvd += bytes_recvd;
    }
    // null terminate buffer
    recv_buffer[expected_len] = '\0';
    return recv_buffer;
}

/*Look into creating a general cleanup that on every fail path just checks every potentially allocated thing and if not null set to null.
s->name, s->major, s, data, recv_buffer, header_buff, also close file and server/client sockets.

VARIABLE_FREES BUT FOR BUFFS! */
