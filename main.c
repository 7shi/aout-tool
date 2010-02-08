#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "a.out.h"

#define OFFSET(a, b) (((char *)(&a->b)) - ((char *)a))
#define INFO(a, b, c, d, e) printf("%08x: %-11s "c"%*.s%s\n", OFFSET(a, b), #b, a->b, 9 - d, "", e)

struct gnu_nlist
{
	long n_strx;
	unsigned char n_type, n_other;
	unsigned short n_desc;
	long n_value;
};

struct gnu_reloc
{
	long r_address;
	unsigned int r_symbolnum:24;
	unsigned int r_pcrel:1;
	unsigned int r_length:2;
	unsigned int r_extern:1;
	unsigned int r_pad:4;
};

void print_symbols(const char *buf, int len)
{
	const struct exec *a = (const struct exec *)buf;
	int start = A_SYMPOS(*a), end = start + a->a_syms, addr = start;
	if (len < end)
	{
		printf("FILE IS TOO SHORT: %08x < %08x\n", len, end);
		return;
	}
	while (addr < end)
	{
		const struct gnu_nlist *nl = (const struct gnu_nlist *)&buf[addr];
		int strad = end + nl->n_strx;
		printf("%08x: %08x n_type=%02x, n_desc=%04x: ",
			addr, nl->n_value, nl->n_type, nl->n_desc);
		if (nl->n_type == 1) printf("extern ");
		if (strad < len)
			printf("%s\n", &buf[strad]);
		else
			printf("???\n");
		addr += sizeof(*nl);
	}
}

void print_relocs(const char *buf, int len, int pos, int rlen)
{
	const struct exec *a = (const struct exec *)buf;
	int sym1 = A_SYMPOS(*a), sym2 = sym1 + a->a_syms;
	int end = pos + rlen, addr = pos;
	if (len < end)
	{
		printf("FILE IS TOO SHORT: %08x < %08x\n", len, end);
		return;
	}
	while (addr < end)
	{
		const struct gnu_reloc *r = (const struct gnu_reloc *)&buf[addr];
		printf("%08x: %08x r_pcrel=%d, r_length=%d: ",
			addr, r->r_address, r->r_pcrel, r->r_length);
		if (r->r_extern)
		{
			int symad = sym1 + r->r_symbolnum * sizeof(struct gnu_nlist);
			printf("extern ");
			if (symad + sizeof(struct gnu_nlist) > len)
				printf("???\n");
			else
			{
				const struct gnu_nlist *nl = (const struct gnu_nlist *)&buf[symad];
				int strad = sym2 + nl->n_strx;
				if (strad < len)
					printf("%s\n", &buf[strad]);
				else
					printf("???\n");
			}
		}
		else if (r->r_length == 2)
		{
			int relad = r->r_address + A_TEXTPOS(*a);
			if (relad < 0 || relad + 4 > len)
				printf("???\n");
			{
				int relto = *(int *)&buf[relad];
				if (r->r_symbolnum == 6)
					printf("data+%08x\n", relto - a->a_text);
				else if (r->r_symbolnum == 8)
					printf("bss+%08x\n", relto - a->a_text - a->a_data);
				else
					printf("text+%08x\n", relto);
			}
		}
		else
			printf("%08x\n", r->r_symbolnum);
		addr += sizeof(*r);
	}
}

void print_aout_header(const char *buf, int len)
{
	const struct exec *a;
	
	if (len < A_MINHDR)
	{
		printf("FILE IS TOO SHORT: %d < %d\n", len, A_MINHDR);
		return;
	}
	
	a = (const struct exec *)buf;
	printf("%08x: %-11s %02x %02x    %s\n",
		OFFSET(a, a_magic), "a_magic", a->a_magic[0], a->a_magic[1], "magic number");
	if (!BADMAG(*a))
		printf("%22s%s\n", "", "a.out executable");
	else if (a->a_magic[0] == 7 && a->a_magic[1] == 1)
		printf("%22s%s\n", "", "a.out object");
	else
	{
		printf("BAD MAGIC NUMBER: %02x %02x (must be %02x %02x)\n",
			a->a_magic[0], a->a_magic[1], A_MAGIC0, A_MAGIC1);
		return;
	}
	
	INFO(a, a_flags, "%02x", 2, "flags, see below");
	if (a->a_flags != 0)
	{
		printf("%21s", "");
		if (a->a_flags & A_UZP  ) printf(" A_UZP");
		if (a->a_flags & A_PAL  ) printf(" A_PAL");
		if (a->a_flags & A_NSYM ) printf(" A_NSYM");
		if (a->a_flags & A_IMG  ) printf(" A_IMG");
		if (a->a_flags & A_EXEC ) printf(" A_EXEC");
		if (a->a_flags & A_SEP  ) printf(" A_SEP");
		if (a->a_flags & A_PURE ) printf(" A_PURE");
		if (a->a_flags & A_TOVLY) printf(" A_TOVLY");
		printf("\n");
	}
	
	INFO(a, a_cpu, "%02x", 2, "cpu id");
	printf("%22s", "");
	switch (a->a_cpu)
	{
		case A_NONE:   printf("A_NONE");   break;
		case A_I8086:  printf("A_I8086");  break;
		case A_M68K:   printf("A_M68K");   break;
		case A_NS16K:  printf("A_NS16K");  break;
		case A_I80386: printf("A_I80386"); break;
		case A_SPARC:  printf("A_SPARC");  break;
		default:       printf("unknown");  break;
	}
	printf(": A_BLR=%d, A_WLR=%d\n", A_BLR(a->a_cpu), A_WLR(a->a_cpu));
	
	INFO(a, a_hdrlen, "%02x", 2, "length of header");
	if (A_HASRELS(*a))
	{
		printf("%21s A_HASRELS", "");
		if (A_HASEXT (*a)) printf(" A_HASEXT");
		if (A_HASLNS (*a)) printf(" A_HASLNS");
		if (A_HASTOFF(*a)) printf(" A_HASTOFF");
		printf("\n");
	}
	
	INFO(a, a_unused,  "%02x", 2, "reserved for future use");
	INFO(a, a_version, "%04x", 4, "version stamp (not used at present)");
	INFO(a, a_text,    "%08x", 8, "size of text segement in bytes");
	INFO(a, a_data,    "%08x", 8, "size of data segment in bytes");
	INFO(a, a_bss,     "%08x", 8, "size of bss segment in bytes");
	INFO(a, a_entry,   "%08x", 8, "entry point");
	INFO(a, a_total,   "%08x", 8, "total memory allocated");
	INFO(a, a_syms,    "%08x", 8, "size of symbol table");
	if (A_HASRELS(*a))
	{
		INFO(a, a_trsize, "%08x", 8, "text relocation size");
		INFO(a, a_drsize, "%08x", 8, "data relocation size");
		if (A_HASEXT(*a))
		{
			INFO(a, a_tbase,  "%08x", 8, "text relocation base");
			INFO(a, a_dbase,  "%08x", 8, "data relocation base");
		}
	}
	
	printf("A_TEXTPOS: %08x %s\n", A_TEXTPOS(*a), "<- a_hdrlen");
	printf("A_DATAPOS: %08x %s\n", A_DATAPOS(*a), "<- A_TEXTPOS + a_text");
	if (A_HASRELS(*a))
	{
		printf("A_TRELPOS: %08x %s\n", A_TRELPOS(*a), "<- A_DATAPOS + a_data");
		printf("A_DRELPOS: %08x %s\n", A_DRELPOS(*a), "<- A_TRELPOS + a_trsize");
	}
	if (a->a_syms > 0)
	{
		if (A_HASRELS(*a))
			printf("A_SYMPOS : %08x %s\n", A_SYMPOS(*a), "<- A_DRELPOS + a_drsize");
		else
			printf("A_SYMPOS : %08x %s\n", A_SYMPOS(*a), "<- A_DATAPOS + a_data");
	}
	if (A_HASRELS(*a))
	{
		if (a->a_trsize > 0)
		{
			printf("[text relocations]\n");
			print_relocs(buf, len, A_TRELPOS(*a), a->a_trsize);
		}
		if (a->a_drsize > 0)
		{
			printf("[data relocations]\n");
			print_relocs(buf, len, A_DRELPOS(*a), a->a_drsize);
		}
	}
	if (a->a_syms > 0)
	{
		printf("[symbols]\n");
		print_symbols(buf, len);
	}
}

void read_aout(const char *file)
{
	FILE *f;
	struct stat st;
	char *buf;
	
	f = fopen(file, "rb");
	if (f == NULL)
	{
		printf("CAN NOT OPEN\n");
		return;
	}
	
	fstat(fileno(f), &st);
	buf = (char *)calloc(1, st.st_size + 1);
	fread(buf, 1, st.st_size, f);
	fclose(f);
	print_aout_header(buf, st.st_size);
	free(buf);
}

int main(int argc, char *argv[])
{
	int i;
	if (argc <= 1)
	{
		fprintf(stderr, "usage: %s a.out [...]\n", argv[0]);
		return 1;
	}
	for (i = 1; i < argc; i++)
	{
		if (i > 1) printf("\n");
		if (argc > 2) printf("==== %s\n", argv[i]);
		read_aout(argv[i]);
	}
	return 0;
}
