struct sauce_record {
    uchar id[5];
    uchar sauce_ver;
    uchar title[35];
    uchar author[20];
    uchar group[20];
    uchar date[8];
    int32_t length;
    uchar datatype;
    uchar filetype'
    uint16_t tinfo1;
    uint16_t tinfo2;
    uint16_t tinfo3;
    uint16_t tinfo4;
    uchar comments;
    uchar flags;
    uchar filler[22]; 
};

