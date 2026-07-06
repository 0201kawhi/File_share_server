#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<sys/socket.h>	//socket
#include<arpa/inet.h>	//inet_addr
#include <unistd.h>


#define PortNum 8888

int main(){

    int sockfd = 0 ;
    struct sockaddr_in server ;

    // 建立 TCP Socket 連線
    sockfd = socket( PF_INET, SOCK_STREAM , 0 ) ;
    if ( sockfd == -1 ){
        printf( "Fail to create a socket\n" ) ;
        return 1 ;
    }
    puts( "Socket created" ) ;

    // 初始化伺服器端位址結構
    bzero( &server, sizeof(server) ) ; //初始化
    server.sin_family = AF_INET ;
    server.sin_addr.s_addr = inet_addr("127.0.0.1") ; 
    server.sin_port = htons(PortNum) ;

    // 連線至遠端伺服器
    if ( connect( sockfd, (struct sockaddr *)&server, sizeof(server)) < 0 ){
        perror("Connect failed") ;
        return 1 ;
    }
    puts( "Connected") ;

    // 顯示指令範例給使用者參考
    printf("=== Command Examples ===\n");
    printf("1. Create file: create <filename> <permission>   (e.g., create test.txt rwxrwx)\n");
    printf("2. Read file:   read <filename>                  (e.g., read test.txt)\n");
    printf("3. Write file:  write <filename> <o/a>           (e.g., write test.txt o)\n");
    printf("4. Changemode:  changemode <filename> <permission> (e.g., changemode test.txt r-r-r-)\n");
    printf("5. Exit:        exit                             (e.g., exit)\n");
    printf("========================\n\n");

    char buffer[1024] ;

    // 接收伺服器回傳的登入提示訊息 (請輸入使用者 1-6)
    recv(sockfd, buffer, sizeof(buffer),0);
    puts(buffer);

    // 讀取使用者輸入的用戶 ID，並過濾換行符號
    fgets(buffer, sizeof(buffer), stdin);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    // 檢查輸入是否為 1 到 6 之間，若不是則要求重新輸入
    while( strcmp(buffer,"1") != 0 && strcmp(buffer,"2") != 0 && strcmp(buffer,"3") != 0 &&
           strcmp(buffer,"4") != 0 && strcmp(buffer,"5") != 0 && strcmp(buffer,"6") != 0 ){

        printf("enter again");
        fgets(buffer, sizeof(buffer), stdin);
        len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }

    // 傳送所選取的用戶 ID 給伺服器端
    send(sockfd, buffer, sizeof(buffer), 0);//send username
    printf("Message sent to server\n");
    
    // 接收伺服器確認回應 (ack)
    recv(sockfd, buffer, sizeof(buffer),0);//ack
    
    // 進入互動選單主循環
    while(1){
        // 顯示支援的功能選項
        char *message = "\n1) create filename permission\n2) read filename\n3) write filename o/a\n4) changemode filename permission\n5) exit\n";
        printf("%s", message);
        
        // 讀取使用者指令輸入
        fgets(buffer, sizeof(buffer), stdin);
        len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        // 傳送指令給伺服器
        send(sockfd, buffer, sizeof(buffer), 0);//command

        // 接收伺服器的處理結果/回應
        recv(sockfd, buffer, sizeof(buffer),0);//response
        
        char command[10][100];
        int word_count = 0;
        
        // 將伺服器回傳的字串分割，解析出回傳狀態與參數
        char *token = strtok(buffer, " ");
        while (token != NULL&& word_count<10) {
            strcpy(command[word_count], token);
            word_count++;

            // 繼續分割
            token = strtok(NULL, " ");
        }

        printf("%s-", buffer);
        
        // 1. 登出指令
        if(strcmp(command[0],"exit") == 0 )
            break;
            
        // 2. 建立檔案指令回應處理
        else if(strcmp(command[0],"create") == 0 ){
            if(strcmp(command[1],"Filealreadyexists") == 0 )
                printf("File already exists\n");
            else if(strcmp(command[1],"success") == 0 )
                printf("File create success\n");
        }
        
        // 3. 讀取檔案指令回應處理
        else if(strcmp(command[0],"read") == 0 ){
            if(strcmp(command[1],"Filenotexists") == 0 )
                printf("File not exists\n");
            else if(strcmp(command[1],"Filebewritten") == 0 )
                printf("File is being written.\n");
            else if(strcmp(command[1],"nopermission") == 0 )
                printf("User does not have permission\n");
            else if(strcmp(command[1],"success") == 0 ){
                // 傳送 ack 表示已準備好接收內容
                send(sockfd, "ack", sizeof("ack"), 0);
                
                // 本地端建立同名檔案以儲存下載的內容
                FILE *file = fopen(command[2], "w");
                
                // 接收檔案內容
                recv(sockfd, buffer, sizeof(buffer),0);//content
                
                // 逐字模擬列印下載的內容（加上微小延遲）
                for(size_t i=0;i<sizeof(buffer);i++){
                    usleep(500000);
                    if(buffer[i]=='\0') break;
                    printf("%c",buffer[i]);
                    fflush(stdout);
                    
                }
                printf("\n");
                // 寫入本地端檔案
                fprintf(file, "%s", buffer);
                fclose(file);
                printf("Read success");
            }

        }
        
        // 4. 寫入檔案指令回應處理
        else if(strcmp(command[0],"write") == 0 ){
            if(strcmp(command[1],"Filenotexists") == 0 )
                printf("File not exists\n");
            else if(strcmp(command[1],"Filebewritten") == 0 )
                printf("File is being written.\n");
            else if(strcmp(command[1],"nopermission") == 0 )
                printf("User does not have permission\n");
            else if(strcmp(command[1],"success") == 0 ){
                // 提示使用者輸入要寫入的內容
                printf("content:");
                fgets(buffer, sizeof(buffer), stdin);
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len - 1] == '\n') {
                    buffer[len - 1] = '\0';
                }

                // 傳送內容給伺服器
                send(sockfd, buffer, sizeof(buffer), 0);//send content
                
                // 等待伺服器確認完成 (ack)
                recv(sockfd, buffer, sizeof(buffer), 0);//ack
                
                printf("write success");
            }
        }
        
        // 5. 修改權限指令回應處理
        else if(strcmp(command[0],"changemode") == 0 ){
            if(strcmp(command[1],"Filealreadyexists") == 0 )
                printf("File already exists\n");
            else if(strcmp(command[1],"success") == 0 ){
                printf("changemode success\n");
            }
        }
    }

    // 關閉 Socket
    close(sockfd);
    return 0 ;

}
