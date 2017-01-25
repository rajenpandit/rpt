#ifndef __ASM__DEFINES_H__31082016_RAJEN__
#define __ASM__DEFINES_H__31082016_RAJEN__

struct stack{
	long int ax;
	long int bx;
	long int cx;
	long int dx;
	long int si;
	long int di;
	long int bp;
	long int sp;
};

#define PUSH(x,y)   __asm__ volatile \
	(\
	 "movq 8(%%rbp),%0;\n"\
	 "\tmovq (%%rbp),%1;\n"\
	 :"=r"(x),"=r"(y)\
	);
#define POP(x,y)    __asm__ volatile \
	(\
	 "movq %0, 8(%%rbp);\n"\
	 "\tmovq %1, (%%rbp);\n"\
	 :\
	 :"r"(x),"r"(y)\
	);
#define PUSH_REGS(sp,si,di,bx,cx)   __asm__ volatile \
        (\
		"movq %%rsp,%0;\n"\
		"\tmovq %%rsi,%1;\n"\
		"\tmovq %%rdi,%2;\n"\
		"\tmovq %%rsi,%3;\n"\
		"\tmovq %%rdi,%4;\n"\
		:"=m"(sp),"=m"(si),"=m"(di),"=m"(bx),"=m"(cx)\
	);

#define POP_REGS(sp,si,di,bx,cx)   __asm__ volatile \
        (\
		"movq %0,%%rsp;\n"\
		"movq %1,%%rsi;\n"\
		"movq %2,%%rdi;\n"\
		"movq %3,%%rbx;\n"\
		"movq %4,%%rcx;\n"\
		:\
		:"m"(sp),"m"(si),"m"(di),"m"(bx),"m"(cx)\
	);
#define GET_PTR(sp,bp)	__asm__ volatile \
	(\
	 	"movq %%rsp,%0;\n"\
	 	"movq %%rbp,%1;\n"\
		:"=m"(sp),"=m"(bp)\
	)
#define SET_PTR(sp,bp)	__asm__ volatile \
	(\
	 	"movq %0,%%rsp;\n"\
	 	"movq %1,%%rbp;\n"\
		:\
		:"m"(sp),"m"(bp)\
	)
#endif
