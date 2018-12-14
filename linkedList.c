void insertIntoList(dirNode* head,char* filename, DIR* stream){
    
    printf("inserted %s dir into LL\n", filename);
    dirNode* newNode = (dirNode*)malloc(sizeof(dirNode));
    newNode -> next = NULL;
    printf("insert1\n");
    newNode -> stream = stream;
    printf("insert2\n");
    newNode -> filename = filename;
    printf("insert3\n");
/*
    newNode -> next = head;
  head = newNode;
  */
    //set temp to the last element of the linked list
    
    dirNode* temp;
    printf("insert4\n");
    while(head != NULL){
        printf("insert5\n");
        temp= head;
        printf("insert6\n");
        head = head -> next; 
        printf("insert7\n");
    }
    printf("insert8\n");
    temp -> next = newNode;
    printf("insert9\n");
    
}

DIR* searchList(dirNode* head, char* filename){
    printf("search1\n");
    dirNode* temp = head -> next;
    printf("search2\n");
    while(temp != NULL){
        printf("search3\n");
        //the strcmp is breaking- why?
        //temp -> filename is null? filename is null?
        printf("temp -> filename is %s\n", temp -> filename);
        printf("filename is %s\n", filename);
        if(!strcmp(temp->filename, filename)){
            printf("search4\n");
            return temp->stream ;
        }
        printf("search5\n");
        temp = temp-> next;
    }
    printf("search6\n");
    return NULL;
}

void deleteFromList(dirNode* head, char* filename){
    printf("delete1\n");
    dirNode* temp = head -> next;
    printf("delete2\n");
    dirNode* prev = head;
    printf("delete3\n");
    while(temp != NULL){
        printf("delete4\n");
        if(!strcmp(temp->filename, filename)){
            printf("delete5\n");
            prev -> next = temp -> next;
            printf("delete6\n");
            free(temp);
            printf("delete7\n");
            return;
        }
        printf("delete8\n");
        prev = temp;
        printf("delete9\n");
        temp = temp -> next;
    }
}
