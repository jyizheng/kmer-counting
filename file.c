/*
BareMetal compile:
GCC (Tested with 4.5.0)

gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o file.o file.c
gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o libBareMetal.o libBareMetal.c
ld -T app.ld -o file.app file.o libBareMetal.o
*/

#include "libBareMetal.h"

#define CPU_NUM 24
#define GEN_PROCESS_NUM 1
#define COUNT_PROCESS_NUM 16
#define COUNT_PAGES 16
//#define COUNT_PAGES 16384
#define FILE_SIZE 13299136
#define KMER_LEN 14

void *kmer_gen_process(void *param);
void *kmer_count_process(void *param);
char *kmer_itoa(unsigned long value, char *result, int base);
void b_output_int_nextline(unsigned long val);
void zero_pages(unsigned long addr, unsigned long nr_pages)

const char *letters = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

unsigned long kmer_gen_processes = GEN_PROCESS_NUM;
unsigned long kmer_count_processes = COUNT_PROCESS_NUM;
unsigned long nr_processes = GEN_PROCESS_NUM + COUNT_PROCESS_NUM;
unsigned long file_buf;


struct work_elem {
	unsigned long index;
};

struct thread_attr {
	unsigned long pid;
	unsigned long queue_addr;
};

struct thread_attr threads_attr[GEN_PROCESS_NUM + COUNT_PROCESS_NUM];

unsigned long thread_arg;
unsigned long k = 0;
unsigned long lock = 0;
unsigned long local = 0;


/* weird behavior: main should be the first */
int main(void)
{
	unsigned long fd, ret;
	unsigned long table;
	unsigned long i;
	unsigned long tval = 0;

	/* hard-coded file size */
	unsigned long cnt = FILE_SIZE;

	/* open the input file */
	fd = b_file_open("data.txt");

	/* allocate mem for input */
	ret = b_mem_allocate(&file_buf, 7);
	if (ret != 7) {
		b_output("allocate mem failed\n");
		return 0;
	}

	/* read from file */
	ret = b_file_read(fd, (void*)(file_buf), cnt);

	/* allocate mem for counters */
	ret = b_mem_allocate(&table, COUNT_PAGES);
	if (ret != COUNT_PAGES) {
		b_output("allocate mem failed\n");
		return 0;
	}

	zero_pages(table, COUNT_PAGES);

	/* allocate memory for per count process work qeueue */
	for (k = 1; k < nr_processes; k++) {
		ret = b_mem_allocate(&threads_attr[k].queue_addr, 1);
		if (ret != 1) {
			b_output("allocate mem failed\n");
			return 0;
		}
		zero_pages(threads_attr[k].queue_addr, 1);
	}

	/* spawn kmer generation process */
	for (k = 0; k < kmer_gen_processes; k++) {
		threads_attr[k].pid = k;
		b_smp_enqueue(kmer_gen_process, (unsigned long)&threads_attr[k]);
	}

	/* spawn kmer counting process */
	for (; k < nr_processes; k++) {
		threads_attr[k].pid = k;
		b_smp_enqueue(kmer_count_process, (unsigned long)&threads_attr[k]);
	}

	/* run all the processes */
	do {
		local = b_smp_dequeue(&thread_arg);
		if (local != 0)
			b_smp_run(local, thread_arg);
	} while (local != 0);
	b_smp_wait();

	/* clean up */
	b_file_close(fd);
	b_mem_release(&file_buf, 7);
	b_mem_release(&table, COUNT_PAGES);

	for (k = 1; k < nr_processes; k++) {
		b_mem_release(&threads_attr[k].queue_addr, 1);
	}

	b_output("I am done\n");
	return 0;
}


char *kmer_itoa(unsigned long value, char *result, int base)
{
	if (base < 2 || base > 36) {
		*result = '\0';
		return result;
	}

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = letters[35 + (tmp_value - value * base)];
	} while (value);

	if (tmp_value < 0) *ptr++ = '-';
		*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

void b_output_int_nextline(unsigned long val)
{
	char str[32];

	kmer_itoa(val, str, 10);
	b_output(str);
	b_output("\n");
}

void b_output_str_nextline(char *str)
{
	b_output(str);
	b_output("\n");
}

static int is_useful_letter(char ch)
{
	if (ch == 'A' || ch == 'G' || ch == 'T' || ch == 'C')
		return 1;

	return 0;
}

/* FIXME: figure out how to handle dot */

unsigned long calculate_pid(char ch1, char ch2)
{
	unsigned long x, y;

	if (ch1 == 'A') x = 0;
	if (ch1 == 'T') x = 1;
	if (ch1 == 'G') x = 2;
	if (ch1 == 'C') x = 3;

	if (ch2 == 'A') y = 0;
	if (ch2 == 'T') y = 1;
	if (ch2 == 'G') y = 2;
	if (ch2 == 'C') y = 3;

	return (x << 2) + y;
}

/* FIXME: how to design the ring buffer */

void add_to_work_queue(unsigned long target_pid,  unsigned long index)
{
	unsigned long work_queue = threads_attr[1 + target_pid];

}

static void kmer_gen(char *line, unsigned long index)
{
	unsigned long target_pid;

	b_output_str_nextline(line);
	
	while (*(line + KMER_LEN - 1) != '\0') {
		target_pid = calculate_pid(*line, *(line + 1));	
		add_to_work_queue(target_pid, index);
		index++;
		line++;
	}
}

void *kmer_gen_process(void *param)
{
	struct thread_attr *arg = (struct thread_attr *)param;
	unsigned long  pid = arg->pid;
	unsigned long index = 0;
	char *str = (char *)file_buf;

	b_output("I am ");
	b_output_int_nextline(pid);

	//while (index <= FILE_SIZE) {
	while (index <= 165 * 3) {
		if (is_useful_letter(str[index]) == 0) {
			index++;
			continue;
		}

		str[index + 150] = '\0';
		kmer_gen(str + index, index);
		index = index + 151;
	}
}

void *kmer_count_process(void *param)
{
	struct thread_attr *arg = (struct thread_attr *)param;
	unsigned long  pid = arg->pid;

	b_output("I am ");
	b_output_int_nextline(pid);
}

void zero_pages(unsigned long addr, unsigned long nr_pages)
{
	unsigned long size = (2 * 1024 * 1024 * nr_pages) / sizeof(unsigned long);
	;unsigned long *buf = (unsigned long) addr;
	int i;

	for (i = 0; i < size; i++)
		buf[i] = 0;
}

