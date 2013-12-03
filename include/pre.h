/********************************************************************************/
/* GCC对C语言的扩展属性，通过在pre.h中对这些属性进行宏定义，可以在使用GCC时使用 */
/* 这些扩展，不使用GCC时就忽略这些扩展。也可以给这些扩展属性重定义一个比较容易  */
/* 记忆的名字								        */
/********************************************************************************/

#ifndef __PRE_H
#define __PRE_H
#if __GNUC__ >=3
//#undef inline

/*指定编译器总是将指定函数内联
 * PS : 不同于C99 inline 关键字只建议不强制
 *函数很小很简单，或者被调用次数不是很多的时候才可以声明为内联
 */
#define inline  inline __attribute ((always_inline))

/*
 * 禁用内联 
 * PS :主动优化模式中,GCC会自动选择适合内联的函数进行内联
 *  但有时使用内联会产生问题如使用__builtin_return_address时内联会产生问题，
 * 因此可以用noinline禁用
 */
#define __noinline     __attribute__ ((noinline))

/*
 *纯函数
 * PS：纯函数是是指没有任何影响的函数，无论调用多少次结果都相同
 *  此类函数可以进行循环优化和子表达式删除.
 *  让纯函数返回void没有意义
 *  for(i =0; i<strlen(p);i++)
 *      printf("%c",toupper(p[i]));
 *  strlen()就是典型的纯函数，如果编译器不知道就可能在每次循环中调用此函数
*/
#define __pure         __attribute__ ((pure))

/* 
 *常函数
 *  PS：更加严格的纯函数，此类函数不可以访问全局变量也不能以指针为参数
 * 此类函数返回oid没有意义
 */
#define __const        __attribute__ ((const))

/* 不返回函数
 * 如果函数不返回，编译器可以进行额外优化
 * 此类函数返回值只能是void
 */
#define __noreturn     __attribute__ ((noreturn))

 
/*
 *分配内存的函数
 *PS：如果函数指针永远不是指向已分配内存的别名，程序员就可以将该函数用malloc标记
 *而编译器也可以进行适当的也优化
 *eg：__attribute__((malloc)) void * getPage(void){
 *        int pageSize = getPageSize();
 *        if(pageSize<=0)
 *             return NULL;
 *        return malloc(pageSize);
 *   }
 */
#define __malloc       __attribute__ ((malloc))

/*
 *强制调用函数检查返回值
 *PS：当被调函数的返回值非常重要的时候，这样处理可以让程序所有调用函数都检查并处理返回值
 *使用warn_unused_result属性修饰的函数不能返回void
 */
#define __must_check   __attribute__ ((warn_unused_result))

/* 
 *将函数标记为deprecated
 *PS：当过时函数被deprecated标记时，如果使用该函数编译器会发出警告
 */
#define __deprecated   __attribute__ ((deprecated))

/* 
 * 将函数标记为used
 * PS：当函数以编译器无法察觉的形式被调用，将函数用used标记可以指示编译器，程序中使用了
 * 该函数，尽管实际上看该函数并未被使用。编译器会输出相应的汇编语言，而且不输出函数未被
 * 使用警告，也不会把函数优化掉。
 * *当函数仅被手写的汇编代码调用时，该属性就非常有帮助了。
 */
#define __used         __attribute__ ((used))

/* 
 * 将函数或参数标记为unused
 * unused属性指示编译器指定的函数或参数未被使用，并指示编译器不产生相应的警告。这个属性
 * 适用于下列情形：
 *     使用了-W或-W编译选项，有一些函数必须匹配事先定好的函数签名，而你还想捕获到真正未被
 *     使用到的函数参数。
 */
#define __unused       __attribute__ ((unused))

/* 
 * 将结构体进行打包
 * PS：packed属性指示编译器对某类型或变量在内存中以尽可能小的空间
 * 保存（打包至内存），这个属性潜在的忽略了内存对其的也要求。
 * 用该属性限定结构题或联合体时，其中所有变量都会被打包。只限定一
 * 个变量时，只有被限定的变量被打包。
 * eg：struct __attribute__ ((packed)) foo（void){
 *        ...
 *     }
 */
#define __packed       __attribute__ ((packed))

/*  
 * 增加变量的内存对齐量
 * PS：改变量指示编译器不使用体系结构和ABI所要求的最小对其量，而使用
 * 不小于指定值的对齐量。
 * eg：下列语句声明整型变量beard_length,并指定至少32byte的内存对齐
 *  int beard_length __attribute__ ((aligned(32))) = 0;
 *  通常指定内存对其量适用于：
 *      1.硬件的内存对齐要求比体系结构更高
 *      2.混写C和汇编代码中希望使用特定内存对齐量的语句时
 *      比如说在保存处理器缓存线中经常使用到的变量，以便优化缓存行为时
 *      就可以使用这个功能。
 */
#define __align(x)     __attribute__ ((aligned(x)))

/* 
 *  如果该属性不加任何参数则是指定编译器使用其能使用的最大内存对其量，
 *  当指定对其量为32byte时，但系统的链接器最多只能对齐8字节，那么变量就
 *  会使用8个字节进行对其。
 *  eg：short parrot_height __attribute__ ((aligned))
 */
#define __align_max    __attribute__ ((aligned))

/* 
 * 分支预测
 * PS：GCC允许程序员对表达式的期望值进行预测，比如告诉编译器，条件语句可能
 * 是真还是假，以便GCC对代码块进行顺序调整等优化。
 */
#define __likely(x)      __builtin_expect (!!(x),1)
#define __unlikely(x)    __builtin_expect (!!(x),0)

#else
#define __noinline 		/* no noinline */
#define __pure			/* no pure */
#define __const 		/* no const */
#define __noreturn		/* no noreturn */
#define __malloc 		/* no malloc */
#define __must_check		/* no warn_unused_result */
#define __deprecated           	/* no deprecated */
#define __used			/* no used */
#define __unused		/* no unused */
#define __packed		/* no packed */
#define __align(x)		/* no aligned */
#define __align_max		/* no aligned_max */
#define __likely(x)       (x)     /* no likely */
#define __unlikely(x)     (x)	  /* no unlikely */
#endif
#endif


