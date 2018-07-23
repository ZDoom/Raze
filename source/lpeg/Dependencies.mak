
$(lpeg_obj)/lpcap.$o: $(addprefix $(lpeg_src)/,lpcap.c lpcap.h lptypes.h)
$(lpeg_obj)/lpcode.$o: $(addprefix $(lpeg_src)/,lpcode.c lptypes.h lpcode.h lptree.h lpvm.h lpcap.h)
$(lpeg_obj)/lpprint.$o: $(addprefix $(lpeg_src)/,lpprint.c lptypes.h lpprint.h lptree.h lpvm.h lpcap.h)
$(lpeg_obj)/lptree.$o: $(addprefix $(lpeg_src)/,lptree.c lptypes.h lpcap.h lpcode.h lptree.h lpvm.h lpprint.h)
$(lpeg_obj)/lpvm.$o: $(addprefix $(lpeg_src)/,lpvm.c lpcap.h lptypes.h lpvm.h lpprint.h lptree.h)
