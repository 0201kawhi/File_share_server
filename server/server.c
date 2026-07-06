#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr 
#include<unistd.h>	//write
#include<pthread.h> //for threading , link with lpthread
#include<netinet/in.h> //INADDR_ANY sockaddr_in 
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<ctype.h>


#define PortNum 8888

void *connection_handler(void *) ;
void ShowCapbilityList() ;

// 檔案資訊結構體，模擬檔案系統中的 Metadata
struct Filelist{

    char name[30] ;      // 檔案名稱
    int owner ;         // 擁有者用戶 ID (1-6)
    char permission[10]; // 權限字串，長度為 6 (例如 "rwr---"，[0,1]=Owner, [2,3]=Group, [4,5]=Other)
    int read;           // 當前正在讀取該檔案的客戶端計數 (模擬共享讀鎖)
    int write;          // 當前正在寫入該檔案的客戶端計數 (模擬互斥寫鎖)
    int status;         // 狀態旗標 (0: 無操作, 1: 讀取中, 2: 寫入中)

};

struct Filelist filelist[500] ; // 儲存所有檔案資訊的陣列
int filenum=0;                  // 目前已建立的檔案總數

int main(){

    int sockfd = 0 ;
    struct sockaddr_in server ,client ;

    // 1. 建立 TCP Socket
    sockfd = socket( PF_INET, SOCK_STREAM, 0 ) ;

    if( sockfd == -1 ){
        printf( "Fail to create a socket\n" ) ;
        return 1 ;
    }    
    puts( "Socket created\n" ) ;

    // 2. 初始化伺服器位址結構
    bzero( &server, sizeof(server) ) ; //初始化
    server.sin_family = AF_INET ; 
    server.sin_addr.s_addr = INADDR_ANY ; // 監聽本機所有網路介面
    server.sin_port = htons(PortNum) ;

    // 3. 綁定 Socket 到指定的連接埠
    if( bind( sockfd,(struct sockaddr*)&server, sizeof(server) ) < 0 ) {
        perror("Fail to bind a server") ;
        return 1 ;
    }
    puts( "Bind done\n") ;

    // 4. 開始監聽，佇列長度設定為 3
    listen( sockfd, 3 ) ;
    puts( "Waiting for connections\n" ) ;

    int clientfd = 0 ;
    int client_len = sizeof(client) ;
    int *create_sock ;

    // 5. 連線接收循環
    while(1) {
        clientfd = accept( sockfd, (struct sockaddr *)&client, (socklen_t*)&client_len ) ;
        puts( "Connection accepted\n" ) ;
        
        pthread_t p_thread ;
        create_sock = malloc(sizeof(int)) ; // 分配記憶體以傳遞套接字描述符給執行緒
        *create_sock = clientfd ;
        
        // 針對每個新的連線，建立一個獨立的執行緒來處理
        if ( pthread_create( &p_thread, NULL, connection_handler, (void*) create_sock) < 0 ){
            perror("Fail to create thread") ;
            free(create_sock);
            return 1 ;
        }
        puts("Handler assigned") ;
    }

    return 0 ;

}

void *connection_handler( void *sockfd ){

    int client_socket = *(int*) sockfd ;
    char buffer[1024];

    int user=0;
    // 提示客戶端登入指定的虛擬用戶 (1 到 6)
    send(client_socket, "Enter an user 1,2,3,4,5,6 : ", sizeof("Enter an user 1,2,3,4,5,6 : "), 0);
    recv(client_socket, buffer, sizeof(buffer),0);
    
    // 解析用戶身份
    if(strcmp(buffer,"1") == 0 )
        user=1;
    else if(strcmp(buffer,"2") == 0 )
        user=2;
    else if(strcmp(buffer,"3") == 0 )
        user=3;
    else if(strcmp(buffer,"4") == 0 )
        user=4;
    else if(strcmp(buffer,"5") == 0 )
        user=5;
    else if(strcmp(buffer,"6") == 0 )
        user=6;
    printf("User %d connected\n", user);
    
    // 回傳確認訊息 (ack)
    send(client_socket, "ack", sizeof("ack"), 0);

    // 處理指令的循環
    while(1){
        // 接收客戶端發送的指令
        recv(client_socket, buffer, sizeof(buffer),0);//command
        
        char command[10][100];
        int word_count = 0;
        
        // 使用 strtok 分割字串，解析出指令參數
        char *token=strtok(buffer, " ");
        while (token != NULL) {
            strcpy(command[word_count], token);
            word_count++;

            // 繼續分割
            token = strtok(NULL, " ");
        }
        
        // 1. exit 指令：結束連線
        if(strcmp(command[0],"exit") == 0 ){
            printf("User %d exited\n", user);
            send(client_socket, "exit", sizeof("exit"), 0);
            break;
        }
        
        // 2. create 指令：建立新檔案
        else if(strcmp(command[0],"create") == 0 ){

            int j=0;
            // 檢查檔案是否已存在於系統列表中
            for(int i=0;i<filenum;i++){
                if(strcmp(command[1],filelist[i].name) == 0 ){
                    j=1;
                    break;
                }
            }
            if(j==1) { 
                send(client_socket, "create Filealreadyexists", sizeof("create Filealreadyexists"), 0);
            }
            else{
                // 新增檔案 Metadata 到列表中
                strcpy(filelist[filenum].name,command[1]);
                filelist[filenum].owner=user;
                filelist[filenum].read=0;
                filelist[filenum].write=0;
                strcpy(filelist[filenum].permission,command[2]);
                filenum++;
                
                // 建立伺服器端實體檔案
                FILE *file = fopen(command[1], "w");//create file
                fclose(file);
                send(client_socket, "create success", sizeof("create success"), 0);
            }
        }
        
        // 3. read 指令：讀取檔案內容
        else if(strcmp(command[0],"read") == 0 ){
            int index=-1;
            // 尋找目標檔案
            for(int i=0;i<filenum;i++){
                if(strcmp(command[1],filelist[i].name) == 0 ){
                    index=i;
                    break;
                }
            }
            // 檢查檔案是否存在
            if(index==-1) send(client_socket, "read Filenotexists", sizeof("read Filenotexists"), 0);
            // 檢查是否正在被寫入 (互斥讀寫)
            else if(filelist[index].write>0)send(client_socket, "read Filebewritten", sizeof("read Filebewritten"), 0);
            else{
                int canread=0;
                // 擁有者自己讀取：檢查第 0 位 'r'
                if(filelist[index].owner==user){
                    if(filelist[index].permission[0]=='r') canread=1;
                }
                // 目前使用者是群組 A (1-3)
                else if(user<=3){
                    // 擁有者也是群組 A：屬於同群組，檢查第 2 位 'r'
                    if(filelist[index].owner<=3){
                        if(filelist[index].permission[2]=='r') canread=1;
                    }
                    // 擁有者是群組 B：屬於其他人，檢查第 4 位 'r'
                    else{
                        if(filelist[index].permission[4]=='r') canread=1;
                    }
                }
                // 目前使用者是群組 B (4-6)
                else{
                    // 擁有者是群組 A：屬於其他人，檢查第 4 位 'r'
                    if(filelist[index].owner<=3){
                        if(filelist[index].permission[4]=='r') canread=1;
                    }
                    // 擁有者也是群組 B：屬於同群組，檢查第 2 位 'r'
                    else{
                        if(filelist[index].permission[2]=='r') canread=1;
                    }
                }

                // 權限檢查不通過
                if(canread==0){
                    send(client_socket, "read nopermission", sizeof("read nopermission"), 0);
                    continue;
                }
                
                // 通過檢查，增加讀取者計數
                filelist[index].read++;
                FILE *file;
                char rbuffer[1024];
                file = fopen(filelist[index].name, "r");
                if (file == NULL) {
                    perror("Error opening file");
                }

                size_t bytesRead = fread(rbuffer, 1, sizeof(rbuffer), file);
                if (ferror(file)) {
                    perror("Error reading file");
                    fclose(file); 
                }
                rbuffer[bytesRead] = '\0';
                fclose(file);
                
                // 回傳讀取成功與檔案名稱給客戶端
                strcpy(buffer,"read success ");
                strcat(buffer,filelist[index].name );
                puts(buffer );
                send(client_socket, buffer, sizeof(buffer), 0);//read success
                
                // 等待客戶端傳送確認接收的 ack
                recv(client_socket, buffer, sizeof(buffer),0);//ack
                
                // 傳送檔案實體內容
                send(client_socket, rbuffer, sizeof(rbuffer), 0);//content
                
                // 讀取完畢，減少讀取者計數
                filelist[index].read--;
            } 

        }
        
        // 4. write 指令：寫入/附加檔案內容
        else if(strcmp(command[0],"write") == 0 ){
            int index=-1;
            // 尋找目標檔案
            for(int i=0;i<filenum;i++){
                if(strcmp(command[1],filelist[i].name) == 0 ){
                    index=i;
                    break;
                }
            }
            // 檢查檔案是否存在
            if(index==-1) send(client_socket, "write Filenotexists", sizeof("write Filenotexists"), 0);
            // 檢查寫入衝突：不能有其他寫入者或正在讀取的使用者
            else if(filelist[index].write>0)send(client_socket, "write Filebewritten", sizeof("write Filebewritten"), 0);
            else if(filelist[index].read>0)send(client_socket, "write Fileberead", sizeof("write Fileberead"), 0);
            else{
                int canwrite=0;
                // 擁有者自己寫入：檢查第 1 位 'w'
                if(filelist[index].owner==user){
                    if(filelist[index].permission[1]=='w') canwrite=1;
                }
                // 目前使用者是群組 A (1-3)
                else if(user<=3){
                    // 擁有者也是群組 A：同群組，檢查第 3 位 'w'
                    if(filelist[index].owner<=3){
                        if(filelist[index].permission[3]=='w') canwrite=1;
                    }
                    // 擁有者是群組 B：其他人，檢查第 5 位 'w'
                    else{
                        if(filelist[index].permission[5]=='w') canwrite=1;
                    }
                }
                // 目前使用者是群組 B (4-6)
                else{
                    // 擁有者是群組 A：其他人，檢查第 5 位 'w'
                    if(filelist[index].owner<=3){
                        if(filelist[index].permission[5]=='w') canwrite=1;
                    }
                    // 擁有者也是群組 B：同群組，檢查第 3 位 'w'
                    else{
                        if(filelist[index].permission[3]=='w') canwrite=1;
                    }
                }

                // 權限檢查不通過
                if(canwrite==0){
                    send(client_socket, "write nopermission", sizeof("write nopermission"), 0);
                    continue;
                }
            
                // 通過檢查，增加寫入者計數 (設為互斥狀態)
                filelist[index].write++;
                FILE *file;

                // 根據模式開啟檔案 (o: 複寫 w, a: 附加 a)
                if(strcmp(command[2],"o") == 0 )
                    file = fopen(filelist[index].name, "w");
                else if(strcmp(command[2],"a") == 0 )
                    file = fopen(filelist[index].name, "a");

                if (file == NULL) {
                    perror("Error opening file");
                }
                
                // 通知客戶端可以開始傳送寫入內容
                strcpy(buffer,"write success ");
                puts(buffer );
                send(client_socket, buffer, sizeof(buffer), 0);//write success
                
                // 接收客戶端寫入內容
                recv(client_socket, buffer, sizeof(buffer),0);//content
                fprintf(file, "%s", buffer);
                fclose(file);
                
                // 模擬處理延遲
                sleep(5);
                
                // 回傳確認訊息 (ack)
                send(client_socket, "ack", sizeof("ack"), 0);//ack
                
                // 寫入完畢，減少寫入者計數
                filelist[index].write--;
            }
        }
        
        // 5. changemode 指令：變更權限
        else if(strcmp(command[0],"changemode") == 0 ){
            int index=-1;
            // 尋找目標檔案
            for(int i=0;i<filenum;i++){
                if(strcmp(command[1],filelist[i].name) == 0 ){
                    index=i;
                    break;
                }
            }
            if(index==-1) send(client_socket, "changemode Filenotexists", sizeof("changemode Filenotexists"), 0);
            else{
                // 變更權限字串
                strcpy(filelist[index].permission,command[2]);
                send(client_socket, "changemode success", sizeof("changemode success"), 0);//changemode success
            }
        }
        else{
            printf("else");
            break;
        }
    }

    // 關閉該執行緒代表的 Client Socket，並釋放動態配置的記憶體
    close(client_socket);
    free(sockfd);
    return NULL;

}

