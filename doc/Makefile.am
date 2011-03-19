man_MANS = libmysqlnb.3 
dist_man_MANS = libmysqlnb.3 

libmysqlnb.3: libmysqlnb.man
	$(NROFF) -man libmysqlnb.man > libmysqlnb.3
	cat libmysqlnb.3 | col -b > libmysqlnb.txt
