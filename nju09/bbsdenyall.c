#include "bbslib.h"

int
bbsdenyall_main()
{
	int i;
	char board[80];
	struct boardmem *brd;
	html_header(1);
	if (!loginok)
		http_fatal("您尚未登录, 请先登录");
	changemode(READING);
	getparmboard(board, sizeof(board));
	if (!(brd = getboard(board)))
		http_fatal("错误的讨论区");
	if (!has_BM_perm(currentuser, brd))
		http_fatal("你无权进行本操作");
	loaddenyuser(board);
	printf("<center>\n");
	printf("%s -- 被封用户名单 [讨论区: %s]<hr><br>\n", BBSNAME, board);
	printf("本版共有 %d 人被封<br>", denynum);
	printf
	    ("<table border=1><tr><td>序号<td>用户帐号<td>被封原因<td>原定解封日期<td>管理\n");
	for (i = 0; i < denynum; i++) {
		printf("<tr><td>%d", i + 1);
		printf("<td><a href=bbsqry?userid=%s>%s</a>", denyuser[i].id,
		       denyuser[i].id);
		printf("<td>%s\n", nohtml(denyuser[i].exp));
		printf("<td>%s\n", Ctime(denyuser[i].free_time) + 4);
		printf
		    ("<td>[<a onclick='return confirm(\"确实解封吗?\")' href=bbsdenydel?B=%d&userid=%s>解封</a>]",
		     getbnumx(brd), denyuser[i].id);
	}
	printf("</table><hr>\n");
	printf
	    ("[<a href=bbsdenyadd?B=%d>设定新的不可POST用户</a>]</center>\n",
	     getbnumx(brd));
	printf
	    ("<p><center><a href=bbsdoc?B=%d>返回版面</a></center>", getbnumx(brd));
	http_quit();
	return 0;
}
