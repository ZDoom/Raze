
$(LPEG_OBJ)/lpcap.$o: $(addprefix $(LPEG_SRC)/,lpcap.c lpcap.h lptypes.h)
$(LPEG_OBJ)/lpcode.$o: $(addprefix $(LPEG_SRC)/,lpcode.c lptypes.h lpcode.h lptree.h lpvm.h lpcap.h)
$(LPEG_OBJ)/lpprint.$o: $(addprefix $(LPEG_SRC)/,lpprint.c lptypes.h lpprint.h lptree.h lpvm.h lpcap.h)
$(LPEG_OBJ)/lptree.$o: $(addprefix $(LPEG_SRC)/,lptree.c lptypes.h lpcap.h lpcode.h lptree.h lpvm.h lpprint.h)
$(LPEG_OBJ)/lpvm.$o: $(addprefix $(LPEG_SRC)/,lpvm.c lpcap.h lptypes.h lpvm.h lpprint.h lptree.h)
