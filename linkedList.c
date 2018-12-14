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
    dirNode* temp = head -> next;

    while(temp != NULL){
        if(!strcmp(temp->filename, filename)){
            return temp->stream ;
        }
        temp = temp-> next;
    }
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
