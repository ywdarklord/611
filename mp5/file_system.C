#include"file_system.H"

/*--------Define File Class------------------*/
FileSystem* File::file_system;

File::File(unsigned int _file_id){
    
	currentpos = 0;
	currentblock = 0;
	file_id = _file_id;
    
	file_system->CreateFile(file_id);
    size = 0;
    start = 0;
	
    
  
}



void File::Reset(){
    currentpos=0;
    currentblock=start;
}

void File::Rewrite(){
    currentblock=0;
    currentpos=0;
    
    unsigned char _buf[512];
	unsigned int block_no = start;
	unsigned int nextblock = 0;
	file_system->Refresh(block_no, _buf, nextblock);
 
}

BOOLEAN File::EoF(){
    return size==currentpos;
}

unsigned int File::Read(unsigned int _n, char *_buf){
    if(_n==0||_buf==NULL||size==0||EoF()) return 0;
	unsigned char tmp_buf[512];
	unsigned int number_data_read = 0;
	
	
	while(_n>0){
		file_system->disk->read(currentblock,tmp_buf);
		unsigned int data_to_read;
        
		
		if(_n>508-currentpos%508) {
			data_to_read = 508-currentpos&508;
		}else{
			data_to_read = _n;
		}
		if(data_to_read>size-currentpos) data_to_read = size-currentpos;
        
		memcpy(_buf+number_data_read,tmp_buf+currentpos%508,data_to_read);
        
		number_data_read += data_to_read;
		_n -= data_to_read;
		currentpos += data_to_read;
	}
	return number_data_read;
}


unsigned int File::Write(unsigned int _n, char *_buf){
    unsigned char buffer[512];
    unsigned int data_written;
    unsigned int data_to_write;
    if (_n==0 || _buf==NULL) {
        return 0;
    }
    int flag=currentpos%508;
    while (_n>0) {
        if (flag==0) {
            if (EoF()) {
                
            }else{
                file_system->disk->read(currentblock,buffer);
                memcpy(&currentpos, buffer+508, 4);
            }
        }
        
        unsigned int offset=currentpos%508;
        file_system->disk->read(currentblock, buffer);
        if (_n>508-offset) {
            data_to_write=508-offset;
        }else{
            data_to_write=_n;
        }
        
        memcpy(buffer+offset, _buf+data_to_write, data_to_write);
        file_system->disk->write(currentblock, buffer);
        _n -= data_to_write;
    }
    
    
    return data_written;
    
}



/*--------File System------------------*/

FileSystem::FileSystem(){
    disk=NULL;
    size=0;
    freeblocks=0;
    File::file_system=this;
}

BOOLEAN FileSystem::Mount(SimpleDisk *_disk){
    disk=_disk;
    unsigned char buffer[512];
    disk->read(0, buffer);
    memcpy(&size, buffer, 4);
    memcpy(&freeblocks, buffer+4, 4);
    memcpy(&numberof_file, buffer+8, 4);
    
    return true;
}

BOOLEAN FileSystem::Format(SimpleDisk *_disk, unsigned int _size){
    unsigned char buffer[512];
    int end=-1;
    memcpy(buffer+508, &end, 4);
    _disk->write(1, buffer);
    
    return true;
}

BOOLEAN FileSystem::CreateFile(int _file_id){
	unsigned int i=0;
	for(; i<512; i++){
		if(data[i][0]==0) break;
	}
	
    data[i][0] = _file_id;
    data[i][1] = 0;
    data[i][2] = 0;
	
    return TRUE;
	
}

BOOLEAN FileSystem::DeleteFile(int _file_id){
    unsigned int i = 0;
	for(;i<512;i++){
		if(data[i][0]==_file_id) break;
	}
	if(i==40) return false;
	unsigned char _buf[512];
	unsigned int block = data[i][2];
	unsigned int next_block = 0;
	for(;;){
		if((int)block==-1) break;
		disk->read(block,_buf);
		memcpy(&next_block,_buf+508,4);
		memcpy(_buf+508,&freeblocks,4);
		disk->write(block,_buf);
		freeblocks = block;
		block = next_block;
	}
	data[i][0] = 0;
	data[i][1] = 0;
	data[i][2] = 0;
	return TRUE;
}

BOOLEAN FileSystem::LookupFile(int _file_id, File * _file) {
    
    for (int i=0; i<512; ++i) {
        if (data[i][0]==_file_id) {
            _file->size=data[i][1];
            return TRUE;
        }
    }
    return FALSE;
}

void FileSystem::Refresh(unsigned int block_no, unsigned char* _buf, unsigned int nextblock){
    while(TRUE){
		if((int)block_no == -1){
			break;
		}
		File::file_system->disk->read(block_no,_buf);
		memcpy(&nextblock, _buf+508, 4);
        
		memcpy(_buf+508, &(File::file_system->freeblocks),4);
		File::file_system->disk->write(block_no,_buf);
		File::file_system->freeblocks = block_no;
        
		block_no = nextblock;
	}
    
}















