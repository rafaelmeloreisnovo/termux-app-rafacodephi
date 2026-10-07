#ifndef RAFZ_NINJA_L0_H
#define RAFZ_NINJA_L0_H
/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (c) 2026 Rafael Melo Reis.
 * RAFCODEPHI RAFNINJA L0: bounded, allocation-free Ninja-subset graph planner.
 * PURE_CORE: no headers, libc, OS services, heap or command execution.
 * NOT a complete Ninja implementation. Unsupported syntax fails closed.
 */
#define RAFN_L0_RULES 12u
#define RAFN_L0_NODES 24u
#define RAFN_L0_INPUTS 8u
#define RAFN_L0_NAME 48u
#define RAFN_L0_COMMAND 160u
#define RAFN_L0_LINE 224u
#define RAFN_L0_FILE 4096u
enum rafn_l0_result {
    RAFN_L0_OK = 0,
    RAFN_L0_INVALID = 1,
    RAFN_L0_LIMIT = 2,
    RAFN_L0_SYNTAX = 3,
    RAFN_L0_DUPLICATE = 4,
    RAFN_L0_UNDEFINED_RULE = 5,
    RAFN_L0_UNDEFINED_DEFAULT = 6,
    RAFN_L0_CYCLE = 7
};
struct rafn_l0_rule { char name[RAFN_L0_NAME]; char command[RAFN_L0_COMMAND]; };
struct rafn_l0_node {
    char output[RAFN_L0_NAME];
    char rule[RAFN_L0_NAME];
    char input[RAFN_L0_INPUTS][RAFN_L0_NAME];
    unsigned input_count;
    unsigned rule_index; /* unsigned -1 for built-in phony */
};
struct rafn_l0_plan {
    struct rafn_l0_rule rules[RAFN_L0_RULES];
    struct rafn_l0_node nodes[RAFN_L0_NODES];
    unsigned order[RAFN_L0_NODES];
    unsigned char mark[RAFN_L0_NODES];
    char default_target[RAFN_L0_NAME];
    unsigned rule_count, node_count, order_count, has_default;
};
static void rafn_l0_zero(void *p, unsigned n) {
    unsigned char *b=(unsigned char *)p; unsigned i;
    for (i=0;i<n;++i) b[i]=0;
}
static int rafn_l0_eq(const char *a, const char *b) {
    unsigned i=0;
    for (;;) { if (a[i]!=b[i]) return 0; if (!a[i]) return 1; ++i; }
}
static int rafn_l0_prefix(const char *s,const char *p) {
    unsigned i=0; while (p[i]) { if (s[i]!=p[i]) return 0; ++i; } return 1;
}
static int rafn_l0_namechar(char c,int path) {
    return ((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
            (c>='0'&&c<='9')||c=='_'||c=='-'||(path&&(c=='.'||c=='/')));
}
/* Reads one fully-delimited token, forbidding expansion, paths escaping
 * the workdir, and Ninja's unsupported variable/implicit/order-only syntax. */
static int rafn_l0_token(const char **cursor,char *out,unsigned cap,int path) {
    const char *s=*cursor; unsigned n=0,seg=0;
    while (*s==' ') ++s;
    if (!*s) return RAFN_L0_SYNTAX;
    if (path && *s=='/') return RAFN_L0_SYNTAX;
    while (*s && *s!=' ' && !(path && *s==':')) {
        if (!rafn_l0_namechar(*s,path)) return RAFN_L0_SYNTAX;
        if (n+1>=cap) return RAFN_L0_LIMIT;
        if (path && *s=='/') {
            if (n==seg || (n-seg==2 && out[seg]=='.' && out[seg+1]=='.')) return RAFN_L0_SYNTAX;
            seg=n+1;
        }
        out[n++]=*s++;
    }
    if (!n || (path && (out[n-1]=='/' || (n-seg==2 && out[seg]=='.'&&out[seg+1]=='.')))) return RAFN_L0_SYNTAX;
    out[n]=0; *cursor=s; return RAFN_L0_OK;
}
static int rafn_l0_eol(const char *s) {
    while (*s==' ') ++s;
    return *s==0;
}
static int rafn_l0_find_node(const struct rafn_l0_plan *p,const char *name) {
    unsigned i; for(i=0;i<p->node_count;++i) if(rafn_l0_eq(p->nodes[i].output,name))return (int)i;
    return -1;
}
static int rafn_l0_find_rule(const struct rafn_l0_plan *p,const char *name) {
    unsigned i; for(i=0;i<p->rule_count;++i) if(rafn_l0_eq(p->rules[i].name,name))return (int)i;
    return -1;
}
static int rafn_l0_visit(struct rafn_l0_plan *p,unsigned i) {
    unsigned j;
    if(p->mark[i]==1) return RAFN_L0_CYCLE;
    if(p->mark[i]==2) return RAFN_L0_OK;
    p->mark[i]=1;
    for(j=0;j<p->nodes[i].input_count;++j) {
        int k=rafn_l0_find_node(p,p->nodes[i].input[j]);
        if(k>=0) {
            int rc=rafn_l0_visit(p,(unsigned)k);
            if(rc) return rc;
        }
    }
    p->mark[i]=2;
    p->order[p->order_count++]=i;
    return RAFN_L0_OK;
}
static int rafn_l0_finalize(struct rafn_l0_plan *p) {
    unsigned i;
    if(!p->node_count) return RAFN_L0_SYNTAX;
    for(i=0;i<p->node_count;++i) {
        int r;
        if(rafn_l0_eq(p->nodes[i].rule,"phony")) {p->nodes[i].rule_index=(unsigned)-1;continue;}
        r=rafn_l0_find_rule(p,p->nodes[i].rule);
        if(r<0 || !p->rules[r].command[0]) return RAFN_L0_UNDEFINED_RULE;
        p->nodes[i].rule_index=(unsigned)r;
    }
    if(p->has_default) {
        int k=rafn_l0_find_node(p,p->default_target);
        if(k<0) return RAFN_L0_UNDEFINED_DEFAULT;
        return rafn_l0_visit(p,(unsigned)k);
    }
    for(i=0;i<p->node_count;++i) {
        int rc=rafn_l0_visit(p,i); if(rc) return rc;
    }
    return RAFN_L0_OK;
}
static int rafn_l0_parse(const char *source,unsigned length,struct rafn_l0_plan *p) {
    unsigned at=0;
    int active_rule=-1;
    if(!source||!p) return RAFN_L0_INVALID;
    rafn_l0_zero(p,(unsigned)sizeof(*p));
    if(!length||length>RAFN_L0_FILE) return RAFN_L0_LIMIT;
    while(at<length) {
        char line[RAFN_L0_LINE+1]; unsigned n=0; const char *s;
        int rc;
        while(at<length && source[at]!='\n') {
            char c=source[at++];
            if(c=='\r' && (at==length||source[at]=='\n')) continue;
            if(c<' ' || c>126) return RAFN_L0_SYNTAX;
            if(n>=RAFN_L0_LINE) return RAFN_L0_LIMIT;
            line[n++]=c;
        }
        if(at<length) ++at;
        line[n]=0; s=line;
        if(!line[0] || line[0]=='#') { active_rule=-1; continue; }
        if(line[0]==' ') {
            unsigned i;
            if(active_rule<0 || !rafn_l0_prefix(s,"  command = ")) return RAFN_L0_SYNTAX;
            s+=12;
            if(!*s) return RAFN_L0_SYNTAX;
            for(i=0;s[i];++i) {
                if(i+1>=RAFN_L0_COMMAND) return RAFN_L0_LIMIT;
                /* Do not silently accept Ninja expansions, which L0 cannot interpret. */
                if(s[i]=='$') return RAFN_L0_SYNTAX;
                p->rules[active_rule].command[i]=s[i];
            }
            p->rules[active_rule].command[i]=0;
            active_rule=-1;
            continue;
        }
        active_rule=-1;
        if(rafn_l0_prefix(s,"rule ")) {
            struct rafn_l0_rule *r; char name[RAFN_L0_NAME];
            s+=5; rc=rafn_l0_token(&s,name,RAFN_L0_NAME,0);
            if(rc) return rc;
            if(!rafn_l0_eol(s)) return RAFN_L0_SYNTAX;
            if(rafn_l0_find_rule(p,name)>=0) return RAFN_L0_DUPLICATE;
            if(p->rule_count>=RAFN_L0_RULES) return RAFN_L0_LIMIT;
            r=&p->rules[p->rule_count++];
            {unsigned k=0;do {r->name[k]=name[k];} while(name[k++]);}
            active_rule=(int)(p->rule_count-1);
        } else if(rafn_l0_prefix(s,"build ")) {
            struct rafn_l0_node *node;
            s+=6;
            if(p->node_count>=RAFN_L0_NODES) return RAFN_L0_LIMIT;
            node=&p->nodes[p->node_count];
            rc=rafn_l0_token(&s,node->output,RAFN_L0_NAME,1);
            if(rc) return rc;
            if(*s!=':') return RAFN_L0_SYNTAX;
            ++s; if(*s!=' ') return RAFN_L0_SYNTAX;
            rc=rafn_l0_token(&s,node->rule,RAFN_L0_NAME,0);
            if(rc) return rc;
            if(rafn_l0_find_node(p,node->output)>=0) return RAFN_L0_DUPLICATE;
            while(*s) {
                while(*s==' ') ++s;
                if(!*s) break;
                if(node->input_count>=RAFN_L0_INPUTS) return RAFN_L0_LIMIT;
                rc=rafn_l0_token(&s,node->input[node->input_count],RAFN_L0_NAME,1);
                if(rc) return rc;
                ++node->input_count;
            }
            ++p->node_count;
        } else if(rafn_l0_prefix(s,"default ")) {
            s+=8;
            if(p->has_default) return RAFN_L0_SYNTAX;
            rc=rafn_l0_token(&s,p->default_target,RAFN_L0_NAME,1);
            if(rc) return rc;
            if(!rafn_l0_eol(s)) return RAFN_L0_SYNTAX;
            p->has_default=1;
        } else return RAFN_L0_SYNTAX;
    }
    return rafn_l0_finalize(p);
}
#endif
