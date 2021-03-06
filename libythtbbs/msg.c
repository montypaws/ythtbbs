#include "ythtbbs.h"

int
save_msgtext(char *uident, struct msghead *head, const char *msgbuf)
{
	char fname[STRLEN], fname2[STRLEN];
	int fd, fd2, i, count, size;
	struct flock ldata;
	struct stat buf;

	sethomefile(fname, uident, "msgindex");
	sethomefile(fname2, uident, "msgcontent");

	if ((fd = open(fname, O_WRONLY | O_CREAT, 0664)) == -1) {
		errlog("msgopen err, %s", uident);
		return -1;	/* 创建文件发生错误 */
	}
	if ((fd2 = open(fname2, O_WRONLY | O_CREAT, 0664)) == -1) {
		errlog("msgopen err, %s", uident);
		close(fd);
		return -1;	/* 创建文件发生错误 */
	}
	ldata.l_type = F_WRLCK;
	ldata.l_whence = 0;
	ldata.l_len = 0;
	ldata.l_start = 0;
	if (fcntl(fd, F_SETLKW, &ldata) == -1) {
		errlog("msglock err, %s", uident);
		close(fd2);
		close(fd);
		return -1;	/* lock error */
	}
	fstat(fd2, &buf);
	size = buf.st_size;
	fstat(fd, &buf);
	count = (buf.st_size - 4) / sizeof (struct msghead);
	if (buf.st_size <= 0) {
		i = 0;
		write(fd, &i, 4);
		count = 0;
	}
	lseek(fd, count * sizeof (struct msghead) + 4, SEEK_SET);
	i = strlen(msgbuf) + 1;
	if (i >= MAX_MSG_SIZE)
		i = MAX_MSG_SIZE - 1;
	head->pos = size;
	head->len = i;
	write(fd, head, sizeof (struct msghead));
	lseek(fd2, size, SEEK_SET);
	write(fd2, msgbuf, i);

	close(fd2);
	ldata.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &ldata);
	close(fd);

	if (!head->sent) {
		sethomefile(fname, uident, "msgindex2");
		if ((fd = open(fname, O_WRONLY | O_CREAT, 0664)) == -1) {
			errlog("msgopen err, %s", uident);
			return -1;	/* 创建文件发生错误 */
		}
		ldata.l_type = F_WRLCK;
		ldata.l_whence = 0;
		ldata.l_len = 0;
		ldata.l_start = 0;
		if (fcntl(fd, F_SETLKW, &ldata) == -1) {
			errlog("msglock err, %s", uident);
			close(fd);
			return -1;	/* lock error */
		}
		fstat(fd, &buf);
		count = (buf.st_size - 4) / sizeof (struct msghead);
		if (buf.st_size <= 0) {
			i = 0;
			write(fd, &i, 4);
			count = 0;
		}
		lseek(fd, count * sizeof (struct msghead) + 4, SEEK_SET);
		write(fd, head, sizeof (struct msghead));
		ldata.l_type = F_UNLCK;
		fcntl(fd, F_SETLKW, &ldata);
		close(fd);
	}
	return 0;
}

int
translate_msg(char *src, struct msghead *head, char *dest, int add_site)
{
	char *time, attstr[STRLEN];
	char uid[STRLEN];
	int i, j = 0, len, pos, ret = 0;
	time = ctime(&head->time);
	dest[0] = 0;
	if (head->mode == 0) {
		if (add_site)
			snprintf(uid, STRLEN, "站长@%s", MY_BBS_ID);
		else
			sprintf(uid, "站长");
	} else {
		if (add_site)
			snprintf(uid, STRLEN, "%s@%s", head->id, MY_BBS_ID);
		else
			sprintf(uid, "%-14.14s", head->id);
	}
	switch (head->mode) {
	case 2:
		if (!head->sent) {
			sprintf(dest,
				"\033[1;44m\033[36m%s\033[33m  (%-16.16s)\033[37m                                              \033[m\n",
				uid, time);
			sprintf(attstr, "\033[44m\033[37;1m");
		} else {
			sprintf(dest,
				"\033[0;1;32m=>\033[37m%s\033[33m(%-16.16s)\033[36m                                              \033[m\n",
				uid, time);
			sprintf(attstr, "\033[36;1m");
		}
		break;
	case 0:
		sprintf(dest,
			"\033[44m\033[33m%s于 %16.16s 时广播\033[37m                                                  \033[m\n",
			uid, time);
		sprintf(attstr, "\033[44m\033[1;37m");
		break;
	case 1:
		if (!head->sent) {
			sprintf(dest,
				"\033[44m\033[36m%-14.14s(%-16.16s) 邀请你\033[37m                                           \033[m\n",
				head->id, time);
			sprintf(attstr, "\033[44m\033[1;37m");
		} else {
			sprintf(dest,
				"\033[44m\033[37m你(%-16.16s) 邀请%-14.14s\033[36m                                           \033[m\n",
				time, head->id);
			sprintf(attstr, "\033[44m\033[1;36m");
		}
		break;
	default:
		sprintf(dest,
			"\033[45m\033[36m%s\033[33m  (\033[36m%-16.16s\033[33m)\033[37m                                              \033[m\n",
			uid, time);
		sprintf(attstr, "\033[45m\033[1;37m");
		break;
	}
	strcat(dest, attstr);
	len = strlen(dest);
	pos = 0;
	for (i = 0; i < strlen(src); i++) {
		if (j)
			j = 0;
		else if (src[i] < 0)
			j = 1;
		if ((j == 0 && pos >= 80) || (j == 1 && pos >= 79)
		    || src[i] == '\n') {
			for (; pos < 79; pos++)
				dest[len++] = ' ';
			dest[len++] = '\033';
			dest[len++] = '[';
			dest[len++] = 'm';
			dest[len++] = '\n';
			ret++;
			for (pos = 0; pos < strlen(attstr); pos++)
				dest[len++] = attstr[pos];
			pos = 0;
			if (src[i] == '\n')
				continue;
		}
		if (src[i] != '\r') {
			dest[len++] = src[i];
			pos++;
		}
	}
	for (; pos < 80; pos++)
		dest[len++] = ' ';
	dest[len++] = '\033';
	dest[len++] = '[';
	dest[len++] = 'm';
	dest[len++] = '\n';
	dest[len] = 0;
	return ret + 2;
}

int
get_unreadcount(char *uident)
{
	char fname[STRLEN];
	int fd, count, ret;
	struct flock ldata;
	struct stat buf;

	sethomefile(fname, uident, "msgindex2");

	if ((fd = open(fname, O_RDONLY, 0664)) == -1) {
		return 0;	/* 创建文件发生错误 */
	}
	ldata.l_type = F_RDLCK;
	ldata.l_whence = 0;
	ldata.l_len = 0;
	ldata.l_start = 0;
	if (fcntl(fd, F_SETLKW, &ldata) == -1) {
		errlog("msglock err, %s", uident);
		close(fd);
		return 0;	/* lock error */
	}
	fstat(fd, &buf);
	count = (buf.st_size - 4) / sizeof (struct msghead);
	if (buf.st_size <= 0)
		ret = 0;
	else {
		read(fd, &ret, 4);
		if (ret >= count)
			ret = 0;
		else
			ret = count - ret;
	}

	ldata.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &ldata);
	close(fd);
	return ret;
}

int
get_msgcount(int id, char *uident)
{
	char fname[STRLEN];
	int fd, count;
	struct flock ldata;
	struct stat buf;
	char idname[10];

	if (id)
		sprintf(idname, "msgindex%d", id + 1);
	else
		strcpy(idname, "msgindex");
	sethomefile(fname, uident, idname);

	if ((fd = open(fname, O_RDONLY, 0664)) == -1) {
		return 0;	/* 创建文件发生错误 */
	}
	ldata.l_type = F_RDLCK;
	ldata.l_whence = 0;
	ldata.l_len = 0;
	ldata.l_start = 0;
	if (fcntl(fd, F_SETLKW, &ldata) == -1) {
		errlog("msglock err, %s", uident);
		close(fd);
		return 0;	/* lock error */
	}
	fstat(fd, &buf);
	count = (buf.st_size - 4) / sizeof (struct msghead);
	ldata.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &ldata);
	close(fd);
	return count;
}

int
load_msghead(int id, char *uident, struct msghead *head, int index)
{
	char fname[STRLEN], idname[10];
	int fd, count;
	struct flock ldata;
	struct stat buf;

	if (id)
		sprintf(idname, "msgindex%d", id + 1);
	else
		strcpy(idname, "msgindex");
	sethomefile(fname, uident, idname);

	if ((fd = open(fname, O_RDONLY, 0664)) == -1) {
		errlog("msgopen err, %s", uident);
		return -1;	/* 创建文件发生错误 */
	}
	ldata.l_type = F_RDLCK;
	ldata.l_whence = 0;
	ldata.l_len = 0;
	ldata.l_start = 0;
	if (fcntl(fd, F_SETLKW, &ldata) == -1) {
		errlog("msglock err, %s", uident);
		close(fd);
		return -1;	/* lock error */
	}
	fstat(fd, &buf);
	count = (buf.st_size - 4) / sizeof (struct msghead);
	if (index < 0 || index >= count) {
		ldata.l_type = F_UNLCK;
		fcntl(fd, F_SETLKW, &ldata);
		close(fd);
		return -1;
	}

	lseek(fd, index * sizeof (struct msghead) + 4, SEEK_SET);
	read(fd, head, sizeof (struct msghead));

	ldata.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &ldata);
	close(fd);
	return 0;
}

int
load_msgtext(char *uident, struct msghead *head, char *msgbuf)
{
	char fname2[STRLEN];
	int fd2, i;

	sethomefile(fname2, uident, "msgcontent");

	msgbuf[0] = 0;

	if ((fd2 = open(fname2, O_RDONLY, 0664)) == -1) {
		errlog("msgopen err, %s", uident);
		return -1;	/* 创建文件发生错误 */
	}
	lseek(fd2, head->pos, SEEK_SET);
	i = head->len;
	if (i >= MAX_MSG_SIZE)
		i = MAX_MSG_SIZE - 1;
	read(fd2, msgbuf, i);
	msgbuf[i] = 0;

	close(fd2);
	return 0;
}

int
get_unreadmsg(char *uident)
{
	char fname[STRLEN];
	int fd, i, count, ret;
#ifndef CYGWIN
	struct flock ldata;
#endif	//CYGWIN
	struct stat buf;

	sethomefile(fname, uident, "msgindex2");

	if ((fd = open(fname, O_RDWR | O_CREAT, 0664)) == -1) {
		errlog("msgopen err, %s", uident);
		return -1;	/* 创建文件发生错误 */
	}
#ifndef CYGWIN
	ldata.l_type = F_RDLCK;
	ldata.l_whence = 0;
	ldata.l_len = 0;
	ldata.l_start = 0;
	if (fcntl(fd, F_SETLKW, &ldata) == -1) {
		errlog("msglock err, %s", uident);
		close(fd);
		return -1;	/* lock error */
	}
#endif	//CYGWIN
	fstat(fd, &buf);
	count = (buf.st_size - 4) / sizeof (struct msghead);
	if (buf.st_size <= 0)
		ret = -1;
	else {
		read(fd, &ret, 4);
		if (ret >= count)
			ret = -1;
		else {
			i = ret + 1;
			lseek(fd, 0, SEEK_SET);
			write(fd, &i, 4);
		}
	}

#ifndef CYGWIN
	ldata.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &ldata);
#endif	//CYGWIN
	close(fd);
	return ret;
}

int
clear_msg(char *uident)
{
	char fname[STRLEN];

	sethomefile(fname, uident, "msgindex");
	unlink(fname);
	sethomefile(fname, uident, "msgindex2");
	unlink(fname);
	sethomefile(fname, uident, "msgcontent");
	unlink(fname);
	return 0;
}
