#make file - use 'make clean' and 'make all' commands to compile
 
clean:  #target name
		$(RM) oss
		$(RM) user_proc
		gcc user_proc.c sharedFunctions.c -o user_proc
		gcc oss.c  sharedFunctions.c -o oss
		
#all:	#oss.c
#		gcc user_proc.c sharedFunctions.c -o user_proc
#		gcc oss.c  sharedFunctions.c -o oss