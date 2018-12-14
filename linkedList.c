void insertIntoList(dirNode* head,char* filename, DIR* stream){
    printf("inserted %s dir into LL\n", filename);
    dirNode* newNode = (dirNode*)malloc(sizeof(dirNode));
    newNode -> stream = stream;
    newNode -> filename = filename;

    //set temp to the last element of the linked list
    dirNode* temp;
    while(head != NULL){
        
        temp= head;
        head = head -> next; 
    }

    temp -> next = newNode;
}

DIR* searchList(dirNode* head, char* filename){
    printf("search1\n");
    dirNode* temp = head -> next;
    printf("search2\n");
    while(temp != NULL){
        printf("search3\n");
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
    dirNode* temp = head -> next;
    dirNode* prev = head;
    while(temp != NULL){
        if(!strcmp(temp->filename, filename)){
            prev -> next = temp -> next;
            free(temp);
            return;
        }
        prev = temp;
        temp = temp -> next;
    }
}
