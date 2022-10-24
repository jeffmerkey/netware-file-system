
/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   Original Authorship      :
*      source code written by Jeff V. Merkey
*
*   Original Contributors    :
*      Jeff V. Merkey
*      
*      
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWCONFIG.C
*   DESCRIP  :   Menu Config Utility
*   DATE     :  November 23, 2022
*
*
***************************************************************************/

#include "globals.h"
#include "nwhelp.h"

extern NWSCREEN ConsoleScreen;
VOLUME *volume_config_table[256];
ULONG volume_config_count = 0;

extern BYTE NwPartSignature[16];
extern BYTE NetwareBootSector[512];
extern ULONG hotfix_table_count;
extern hotfix_info_table hotfix_table[256];
extern ULONG segment_table_count;
extern segment_info_table segment_table[256];
extern ULONG segment_mark_table[256];
extern ULONG FirstValidDisk;

#if (LINUX_UTIL)
extern BYTE *PhysicalDiskNames[];
extern ULONG MaxHandlesSupported;
#endif

extern BYTE *get_block_descrip(ULONG Type);
extern ULONG get_block_value(ULONG Type);
extern BYTE *get_ns_string(ULONG);
extern BYTE *NSDescription(ULONG ns);
extern void build_segment_tables(void);

extern ULONG UtilInitializeDirAssignHash(VOLUME *volume);
extern ULONG UtilCreateDirAssignEntry(VOLUME *volume, ULONG Parent, ULONG BlockNo,
			   DOS *dos);
extern ULONG UtilFreeDirAssignHash(VOLUME *volume);
extern ULONG UtilAddToDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
extern ULONG UtilRemoveDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
extern ULONG UtilAllocateDirectoryRecord(VOLUME *volume, ULONG Parent);
extern ULONG UtilFreeDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo,
			      ULONG Parent);
extern ULONG FormatNamespaceRecord(VOLUME *volume, BYTE *Name, ULONG Len,
			    ULONG Parent, ULONG NWFlags, ULONG Namespace,
			    DOS *dos);

ULONG menu = 0, portal = 0, mportal = 0, nportal = 0, pportal = 0;
ULONG CurrentDisk = 0;

BYTE *YesNo[2]=
{
   "Yes",
   "No "
};

BYTE *OnOff[2]=
{
   "On ",
   "Off"
};

BYTE *VolumeType[2]=
{
   "3.x    ",
   "4.x/5.x"
};

BYTE *BlockSizes[5]=
{
   "4K  Blocks",
   "8K  Blocks",
   "16K Blocks",
   "32K Blocks",
   "64K Blocks",
};

#if (DOS_UTIL | LINUX_UTIL | WINDOWS_NT_UTIL)

BYTE *disk_name_array[]=
{
    "Disk (00)    ", "Disk (01)    ", "Disk (02)    ", "Disk (03)    ",
    "Disk (04)    ", "Disk (05)    ", "Disk (06)    ", "Disk (07)    ",
    "Disk (08)    ", "Disk (09)    ", "Disk (10)    ", "Disk (11)    ",
    "Disk (12)    ", "Disk (13)    ", "Disk (14)    ", "Disk (15)    ",
    "Disk (16)    ", "Disk (17)    ", "Disk (18)    ", "Disk (19)    ",
    "Disk (20)    ", "Disk (21)    ", "Disk (22)    ", "Disk (23)    ",
    "Disk (24)    ", "Disk (25)    ", "Disk (26)    ", "Disk (27)    ",
    "Disk (28)    ", "Disk (29)    ", "Disk (30)    ", "Disk (31)    ",
    "Disk (32)    ", "Disk (33)    ", "Disk (34)    ", "Disk (35)    ",
    "Disk (36)    ", "Disk (37)    ", "Disk (38)    ", "Disk (39)    ",
    "Disk (40)    ", "Disk (41)    ", "Disk (42)    ", "Disk (43)    ",
    "Disk (44)    ", "Disk (45)    ", "Disk (46)    ", "Disk (47)    ",
    "Disk (48)    ", "Disk (49)    ", "Disk (50)    ", "Disk (51)    ",
    "Disk (52)    ", "Disk (53)    ", "Disk (54)    ", "Disk (55)    ",
    "Disk (56)    ", "Disk (57)    ", "Disk (58)    ", "Disk (59)    ",
    "Disk (60)    ", "Disk (61)    ", "Disk (62)    ", "Disk (63)    ",
    "Disk (64)    ", "Disk (65)    ", "Disk (66)    ", "Disk (67)    ",
    "Disk (68)    ", "Disk (69)    ", "Disk (70)    ", "Disk (71)    ",
    "Disk (72)    ", "Disk (73)    ", "Disk (74)    ", "Disk (75)    ",
    "Disk (76)    ", "Disk (77)    ", "Disk (78)    ", "Disk (79)    ",
    "Disk (80)    ", "Disk (81)    ", "Disk (82)    ", "Disk (83)    ",
    "Disk (84)    ", "Disk (85)    ", "Disk (86)    ", "Disk (87)    ",
    "Disk (88)    ", "Disk (89)    ", "Disk (90)    ", "Disk (91)    ",
    "Disk (92)    ", "Disk (93)    ", "Disk (94)    ", "Disk (95)    ",
    "Disk (96)    ", "Disk (97)    ", "Disk (98)    ", "Disk (99)    ",

    "Disk (100)    ", "Disk (101)    ", "Disk (102)    ", "Disk (103)    ",
    "Disk (104)    ", "Disk (105)    ", "Disk (106)    ", "Disk (107)    ",
    "Disk (108)    ", "Disk (109)    ", "Disk (110)    ", "Disk (111)    ",
    "Disk (112)    ", "Disk (113)    ", "Disk (114)    ", "Disk (115)    ",
    "Disk (116)    ", "Disk (117)    ", "Disk (118)    ", "Disk (119)    ",
    "Disk (120)    ", "Disk (121)    ", "Disk (122)    ", "Disk (123)    ",
    "Disk (124)    ", "Disk (125)    ", "Disk (126)    ", "Disk (127)    ",
    "Disk (128)    ", "Disk (129)    ", "Disk (130)    ", "Disk (131)    ",
    "Disk (132)    ", "Disk (133)    ", "Disk (134)    ", "Disk (135)    ",
    "Disk (136)    ", "Disk (137)    ", "Disk (138)    ", "Disk (139)    ",
    "Disk (140)    ", "Disk (141)    ", "Disk (142)    ", "Disk (143)    ",
    "Disk (144)    ", "Disk (145)    ", "Disk (146)    ", "Disk (147)    ",
    "Disk (148)    ", "Disk (149)    ", "Disk (150)    ", "Disk (151)    ",
    "Disk (152)    ", "Disk (153)    ", "Disk (154)    ", "Disk (155)    ",
    "Disk (156)    ", "Disk (157)    ", "Disk (158)    ", "Disk (159)    ",
    "Disk (160)    ", "Disk (161)    ", "Disk (162)    ", "Disk (163)    ",
    "Disk (164)    ", "Disk (165)    ", "Disk (166)    ", "Disk (167)    ",
    "Disk (168)    ", "Disk (169)    ", "Disk (170)    ", "Disk (171)    ",
    "Disk (172)    ", "Disk (173)    ", "Disk (174)    ", "Disk (175)    ",
    "Disk (176)    ", "Disk (177)    ", "Disk (178)    ", "Disk (179)    ",
    "Disk (180)    ", "Disk (181)    ", "Disk (182)    ", "Disk (183)    ",
    "Disk (184)    ", "Disk (185)    ", "Disk (186)    ", "Disk (187)    ",
    "Disk (188)    ", "Disk (189)    ", "Disk (190)    ", "Disk (191)    ",
    "Disk (192)    ", "Disk (193)    ", "Disk (194)    ", "Disk (195)    ",
    "Disk (196)    ", "Disk (197)    ", "Disk (198)    ", "Disk (199)    ",

    "Disk (200)    ", "Disk (201)    ", "Disk (202)    ", "Disk (203)    ",
    "Disk (204)    ", "Disk (205)    ", "Disk (206)    ", "Disk (207)    ",
    "Disk (208)    ", "Disk (209)    ", "Disk (210)    ", "Disk (211)    ",
    "Disk (212)    ", "Disk (213)    ", "Disk (214)    ", "Disk (215)    ",
    "Disk (216)    ", "Disk (217)    ", "Disk (218)    ", "Disk (219)    ",
    "Disk (220)    ", "Disk (221)    ", "Disk (222)    ", "Disk (223)    ",
    "Disk (224)    ", "Disk (225)    ", "Disk (226)    ", "Disk (227)    ",
    "Disk (228)    ", "Disk (229)    ", "Disk (230)    ", "Disk (231)    ",
    "Disk (232)    ", "Disk (233)    ", "Disk (234)    ", "Disk (235)    ",
    "Disk (236)    ", "Disk (237)    ", "Disk (238)    ", "Disk (239)    ",
    "Disk (240)    ", "Disk (241)    ", "Disk (242)    ", "Disk (243)    ",
    "Disk (244)    ", "Disk (245)    ", "Disk (246)    ", "Disk (247)    ",
    "Disk (248)    ", "Disk (249)    ", "Disk (250)    ", "Disk (251)    ",
    "Disk (252)    ", "Disk (253)    ", "Disk (254)    ", "Disk (255)    ",
    "Disk (256)    ", "Disk (257)    ", "Disk (258)    ", "Disk (259)    ",
    "Disk (260)    ", "Disk (261)    ", "Disk (262)    ", "Disk (263)    ",
    "Disk (264)    ", "Disk (265)    ", "Disk (266)    ", "Disk (267)    ",
    "Disk (268)    ", "Disk (269)    ", "Disk (270)    ", "Disk (271)    ",
    "Disk (272)    ", "Disk (273)    ", "Disk (274)    ", "Disk (275)    ",
    "Disk (276)    ", "Disk (277)    ", "Disk (278)    ", "Disk (279)    ",
    "Disk (280)    ", "Disk (281)    ", "Disk (282)    ", "Disk (283)    ",
    "Disk (284)    ", "Disk (285)    ", "Disk (286)    ", "Disk (287)    ",
    "Disk (288)    ", "Disk (289)    ", "Disk (290)    ", "Disk (291)    ",
    "Disk (292)    ", "Disk (293)    ", "Disk (294)    ", "Disk (295)    ",
    "Disk (296)    ", "Disk (297)    ", "Disk (298)    ", "Disk (299)    ",

    "Disk (300)    ", "Disk (301)    ", "Disk (302)    ", "Disk (303)    ",
    "Disk (304)    ", "Disk (305)    ", "Disk (306)    ", "Disk (307)    ",
    "Disk (308)    ", "Disk (309)    ", "Disk (310)    ", "Disk (311)    ",
    "Disk (312)    ", "Disk (313)    ", "Disk (314)    ", "Disk (315)    ",
    "Disk (316)    ", "Disk (317)    ", "Disk (318)    ", "Disk (319)    ",
    "Disk (320)    ", "Disk (321)    ", "Disk (322)    ", "Disk (323)    ",
    "Disk (324)    ", "Disk (325)    ", "Disk (326)    ", "Disk (327)    ",
    "Disk (328)    ", "Disk (329)    ", "Disk (330)    ", "Disk (331)    ",
    "Disk (332)    ", "Disk (333)    ", "Disk (334)    ", "Disk (335)    ",
    "Disk (336)    ", "Disk (337)    ", "Disk (338)    ", "Disk (339)    ",
    "Disk (340)    ", "Disk (341)    ", "Disk (342)    ", "Disk (343)    ",
    "Disk (344)    ", "Disk (345)    ", "Disk (346)    ", "Disk (347)    ",
    "Disk (348)    ", "Disk (349)    ", "Disk (350)    ", "Disk (351)    ",
    "Disk (352)    ", "Disk (353)    ", "Disk (354)    ", "Disk (355)    ",
    "Disk (356)    ", "Disk (357)    ", "Disk (358)    ", "Disk (359)    ",
    "Disk (360)    ", "Disk (361)    ", "Disk (362)    ", "Disk (363)    ",
    "Disk (364)    ", "Disk (365)    ", "Disk (366)    ", "Disk (367)    ",
    "Disk (368)    ", "Disk (369)    ", "Disk (370)    ", "Disk (371)    ",
    "Disk (372)    ", "Disk (373)    ", "Disk (374)    ", "Disk (375)    ",
    "Disk (376)    ", "Disk (377)    ", "Disk (378)    ", "Disk (379)    ",
    "Disk (380)    ", "Disk (381)    ", "Disk (382)    ", "Disk (383)    ",
    "Disk (384)    ", "Disk (385)    ", "Disk (386)    ", "Disk (387)    ",
    "Disk (388)    ", "Disk (389)    ", "Disk (390)    ", "Disk (391)    ",
    "Disk (392)    ", "Disk (393)    ", "Disk (394)    ", "Disk (395)    ",
    "Disk (396)    ", "Disk (397)    ", "Disk (398)    ", "Disk (399)    ",

    "Disk (400)    ", "Disk (401)    ", "Disk (402)    ", "Disk (403)    ",
    "Disk (404)    ", "Disk (405)    ", "Disk (406)    ", "Disk (407)    ",
    "Disk (408)    ", "Disk (409)    ", "Disk (410)    ", "Disk (411)    ",
    "Disk (412)    ", "Disk (413)    ", "Disk (414)    ", "Disk (415)    ",
    "Disk (416)    ", "Disk (417)    ", "Disk (418)    ", "Disk (419)    ",
    "Disk (420)    ", "Disk (421)    ", "Disk (422)    ", "Disk (423)    ",
    "Disk (424)    ", "Disk (425)    ", "Disk (426)    ", "Disk (427)    ",
    "Disk (428)    ", "Disk (429)    ", "Disk (430)    ", "Disk (431)    ",
    "Disk (432)    ", "Disk (433)    ", "Disk (434)    ", "Disk (435)    ",
    "Disk (436)    ", "Disk (437)    ", "Disk (438)    ", "Disk (439)    ",
    "Disk (440)    ", "Disk (441)    ", "Disk (442)    ", "Disk (443)    ",
    "Disk (444)    ", "Disk (445)    ", "Disk (446)    ", "Disk (447)    ",
    "Disk (448)    ", "Disk (449)    ", "Disk (450)    ", "Disk (451)    ",
    "Disk (452)    ", "Disk (453)    ", "Disk (454)    ", "Disk (455)    ",
    "Disk (456)    ", "Disk (457)    ", "Disk (458)    ", "Disk (459)    ",
    "Disk (460)    ", "Disk (461)    ", "Disk (462)    ", "Disk (463)    ",
    "Disk (464)    ", "Disk (465)    ", "Disk (466)    ", "Disk (467)    ",
    "Disk (468)    ", "Disk (469)    ", "Disk (470)    ", "Disk (471)    ",
    "Disk (472)    ", "Disk (473)    ", "Disk (474)    ", "Disk (475)    ",
    "Disk (476)    ", "Disk (477)    ", "Disk (478)    ", "Disk (479)    ",
    "Disk (480)    ", "Disk (481)    ", "Disk (482)    ", "Disk (483)    ",
    "Disk (484)    ", "Disk (485)    ", "Disk (486)    ", "Disk (487)    ",
    "Disk (488)    ", "Disk (489)    ", "Disk (490)    ", "Disk (491)    ",
    "Disk (492)    ", "Disk (493)    ", "Disk (494)    ", "Disk (495)    ",
    "Disk (496)    ", "Disk (497)    ", "Disk (498)    ", "Disk (499)    ",

    "Disk (500)    ", "Disk (501)    ", "Disk (502)    ", "Disk (503)    ",
    "Disk (504)    ", "Disk (505)    ", "Disk (506)    ", "Disk (507)    ",
    "Disk (508)    ", "Disk (509)    ", "Disk (510)    ", "Disk (511)    ",
    "Disk (512)    ", "Disk (513)    ", "Disk (514)    ", "Disk (515)    ",
    "Disk (516)    ", "Disk (517)    ", "Disk (518)    ", "Disk (519)    ",
    "Disk (520)    ", "Disk (521)    ", "Disk (522)    ", "Disk (523)    ",
    "Disk (524)    ", "Disk (525)    ", "Disk (526)    ", "Disk (527)    ",
    "Disk (528)    ", "Disk (529)    ", "Disk (530)    ", "Disk (531)    ",
    "Disk (532)    ", "Disk (533)    ", "Disk (534)    ", "Disk (535)    ",
    "Disk (536)    ", "Disk (537)    ", "Disk (538)    ", "Disk (539)    ",
    "Disk (540)    ", "Disk (541)    ", "Disk (542)    ", "Disk (543)    ",
    "Disk (544)    ", "Disk (545)    ", "Disk (546)    ", "Disk (547)    ",
    "Disk (548)    ", "Disk (549)    ", "Disk (550)    ", "Disk (551)    ",
    "Disk (552)    ", "Disk (553)    ", "Disk (554)    ", "Disk (555)    ",
    "Disk (556)    ", "Disk (557)    ", "Disk (558)    ", "Disk (559)    ",
    "Disk (560)    ", "Disk (561)    ", "Disk (562)    ", "Disk (563)    ",
    "Disk (564)    ", "Disk (565)    ", "Disk (566)    ", "Disk (567)    ",
    "Disk (568)    ", "Disk (569)    ", "Disk (570)    ", "Disk (571)    ",
    "Disk (572)    ", "Disk (573)    ", "Disk (574)    ", "Disk (575)    ",
    "Disk (576)    ", "Disk (577)    ", "Disk (578)    ", "Disk (579)    ",
    "Disk (580)    ", "Disk (581)    ", "Disk (582)    ", "Disk (583)    ",
    "Disk (584)    ", "Disk (585)    ", "Disk (586)    ", "Disk (587)    ",
    "Disk (588)    ", "Disk (589)    ", "Disk (590)    ", "Disk (591)    ",
    "Disk (592)    ", "Disk (593)    ", "Disk (594)    ", "Disk (595)    ",
    "Disk (596)    ", "Disk (597)    ", "Disk (598)    ", "Disk (599)    ",

    "Disk (600)    ", "Disk (601)    ", "Disk (602)    ", "Disk (603)    ",
    "Disk (604)    ", "Disk (605)    ", "Disk (606)    ", "Disk (607)    ",
    "Disk (608)    ", "Disk (609)    ", "Disk (610)    ", "Disk (611)    ",
    "Disk (612)    ", "Disk (613)    ", "Disk (614)    ", "Disk (615)    ",
    "Disk (616)    ", "Disk (617)    ", "Disk (618)    ", "Disk (619)    ",
    "Disk (620)    ", "Disk (621)    ", "Disk (622)    ", "Disk (623)    ",
    "Disk (624)    ", "Disk (625)    ", "Disk (626)    ", "Disk (627)    ",
    "Disk (628)    ", "Disk (629)    ", "Disk (630)    ", "Disk (631)    ",
    "Disk (632)    ", "Disk (633)    ", "Disk (634)    ", "Disk (635)    ",
    "Disk (636)    ", "Disk (637)    ", "Disk (638)    ", "Disk (639)    ",
    "Disk (640)    ", "Disk (641)    ", "Disk (642)    ", "Disk (643)    ",
    "Disk (644)    ", "Disk (645)    ", "Disk (646)    ", "Disk (647)    ",
    "Disk (648)    ", "Disk (649)    ", "Disk (650)    ", "Disk (651)    ",
    "Disk (652)    ", "Disk (653)    ", "Disk (654)    ", "Disk (655)    ",
    "Disk (656)    ", "Disk (657)    ", "Disk (658)    ", "Disk (659)    ",
    "Disk (660)    ", "Disk (661)    ", "Disk (662)    ", "Disk (663)    ",
    "Disk (664)    ", "Disk (665)    ", "Disk (666)    ", "Disk (667)    ",
    "Disk (668)    ", "Disk (669)    ", "Disk (670)    ", "Disk (671)    ",
    "Disk (672)    ", "Disk (673)    ", "Disk (674)    ", "Disk (675)    ",
    "Disk (676)    ", "Disk (677)    ", "Disk (678)    ", "Disk (679)    ",
    "Disk (680)    ", "Disk (681)    ", "Disk (682)    ", "Disk (683)    ",
    "Disk (684)    ", "Disk (685)    ", "Disk (686)    ", "Disk (687)    ",
    "Disk (688)    ", "Disk (689)    ", "Disk (690)    ", "Disk (691)    ",
    "Disk (692)    ", "Disk (693)    ", "Disk (694)    ", "Disk (695)    ",
    "Disk (696)    ", "Disk (697)    ", "Disk (698)    ", "Disk (699)    ",

    NULL,

};

void ScanVolumes(void)
{
    register ULONG message;

    message = create_message_portal(" Scanning Disks ... ", 12,
				    YELLOW | BGBLUE | BLINK);
    NWFSVolumeScan();
    if (message)
       close_message_portal(message);
}

BYTE pportal_header[255];

extern ULONG input_get_number(ULONG *new_value);
extern ULONG convert_volume_name_to_handle(BYTE *volume_name, ULONG *vpart_handle);

void	nwconfig_build_hotfix_tables(void)
{
    ULONG j, i;
    ULONG	group_number = 0;
    ULONG	vhandle;
    ULONG	logical_capacity;
    ULONG	raw_ids[4];
    nwvp_payload payload;
    extern void ScanVolumes(void);
    
    nwvp_rpartition_scan_info 		rlist[5];
    nwvp_rpartition_info	 	rpart_info;
    nwvp_lpartition_scan_info 		llist[5];
    nwvp_lpartition_info 		lpart_info;

    ScanVolumes();

    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (sizeof(nwvp_rpartition_scan_info) << 20) + sizeof(rlist);
    payload.payload_buffer = (BYTE *) &rlist[0];
    do
    {
	nwvp_rpartition_scan(&payload);
	for (j=0; j< payload.payload_object_count; j++)
	{
	    nwvp_rpartition_return_info(rlist[j].rpart_handle, &rpart_info);
	    if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
	    {
		if (rpart_info.lpart_handle == 0xFFFFFFFF)
		{
		   logical_capacity = (rpart_info.physical_block_count & 0xFFFFFF00) - 242;
		   raw_ids[0] = rpart_info.rpart_handle;
		   raw_ids[1] = 0xFFFFFFFF;
		   raw_ids[2] = 0xFFFFFFFF;
		   raw_ids[3] = 0xFFFFFFFF;
		   nwvp_lpartition_format(&vhandle, logical_capacity, &raw_ids[0]);
		}
	    }
	}
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

    hotfix_table_count = 0;
    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_scan_info) << 20) + sizeof(llist);
    payload.payload_buffer = (BYTE *) &llist[0];
    do
    {
       nwvp_lpartition_scan(&payload);
       for (j=0; j< payload.payload_object_count; j++)
       {
	  if (nwvp_lpartition_return_info(llist[j].lpart_handle, &lpart_info) == 0)
	  {
	     for (i=0; i<lpart_info.mirror_count; i++)
	     {
		if (hotfix_table_count <= 256)
		{
		   hotfix_table[hotfix_table_count].group_number = group_number;
		   hotfix_table[hotfix_table_count].rpart_handle = lpart_info.m[i].rpart_handle;
		   hotfix_table[hotfix_table_count].lpart_handle = llist[j].lpart_handle;
		   hotfix_table[hotfix_table_count].physical_block_count = lpart_info.logical_block_count + lpart_info.m[i].hotfix_block_count;
		   hotfix_table[hotfix_table_count].logical_block_count = lpart_info.logical_block_count;
		   hotfix_table[hotfix_table_count].segment_count = lpart_info.segment_count;
		   hotfix_table[hotfix_table_count].mirror_count = lpart_info.mirror_count;
		   hotfix_table[hotfix_table_count].hotfix_block_count = ((lpart_info.m[i].status & MIRROR_PRESENT_BIT) == 0) ? 0 : lpart_info.m[i].hotfix_block_count;
		   hotfix_table[hotfix_table_count].insynch_flag = ((lpart_info.m[i].status & MIRROR_INSYNCH_BIT) == 0) ? 0 : 0xFFFFFFFF;
		   hotfix_table[hotfix_table_count].active_flag = ((lpart_info.lpartition_status & MIRROR_GROUP_ACTIVE_BIT) == 0) ? 0 : 0xFFFFFFFF;
		   hotfix_table_count ++;
		}
	     }
	     group_number ++;
	  }
       }
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
}

ULONG NWConfigAllocateUnpartitionedSpace(void)
{
    ULONG disk, i, j, vhandle, raw_ids[4], ccode;
    ULONG TotalSectors, FreeSectors, AdjustedSectors;
    ULONG AllocatedSectors, freeSlots, message;
    ULONG LBAStart, Length, LBAWork, LBAEnd;
    BYTE *Buffer;
    register ULONG retCode;
    register PART_SIG *ps;
    ULONG SlotCount;
    ULONG SlotIndex;
    ULONG SlotMask[4];
    ULONG SlotLBA[4];
    ULONG SlotLength[4];
    ULONG FreeCount;
    ULONG FreeLBA[4];
    ULONG FreeLength[4];
    ULONG FreeSlotIndex[4];
    nwvp_rpartition_scan_info	rlist[5];
    nwvp_payload payload;
    ULONG logical_capacity;
    BYTE displayBuffer[255];

    Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Buffer)
    {
       error_portal("could not allocate partition workspace", 10);
       return -1;
    }
    NWFSSet(Buffer, 0, IO_BLOCK_SIZE);

    ScanVolumes();

    ccode = confirm_menu("  Assign All Free Space to NetWare Partitions?",
			 10, YELLOW | BGBLUE);
    if (!ccode)
       return -1;

    message = create_message_portal("Allocating all free space for NW4.x/5.x", 10, YELLOW | BGBLUE);
    for (disk=0; disk < TotalDisks; disk++)
    {
       if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
       {
	  // init the partition marker fields
	  for (i=0; i < 4; i++)
	     SystemDisk[disk]->PartitionFlags[i] = 0;

	  if (ValidatePartitionExtants(disk))
	  {
	     log_sprintf(displayBuffer,
		    "disk-(%d) partition table is invalid - reinitializing table",
		    (int)disk);
	     error_portal(displayBuffer, 10);

	     ccode = confirm_menu("  This Will Erase This drive.  Continue ? ",
				  10, YELLOW | BGBLUE);
	     if (!ccode)
                goto ScanNextDisk;

	     retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
		log_sprintf(displayBuffer,
			"FATAL:  disk-%d error reading boot sector", (int)disk);
		error_portal(displayBuffer, 10);
		goto ScanNextDisk;
	     }
	     NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	     // if there is no valid partition signature, then
	     // write a new master boot record.

	     if (SystemDisk[disk]->partitionSignature != 0xAA55)
	     {
		NWFSSet(Buffer, 0, 512);
		NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		SystemDisk[disk]->partitionSignature = 0xAA55;
		NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	     }

	     // zero the partition table and partition signature
	     NWFSSet(&SystemDisk[disk]->PartitionTable[0].fBootable, 0, 64);
	     NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

	     // write the partition table to sector 0
	     retCode = WriteDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
		log_sprintf(displayBuffer,
			"FATAL:  disk-%d error writing boot sector", (int)disk);
		error_portal(displayBuffer, 10);
		goto ScanNextDisk;
	     }
	     ScanVolumes();
	  }

          //
          //
          //

	  TotalSectors = (ULONG)((SystemDisk[disk]->Cylinders) *
			         SystemDisk[disk]->TracksPerCylinder *
			         SystemDisk[disk]->SectorsPerTrack);

	  if (TotalSectors < SystemDisk[disk]->SectorsPerTrack)
	  {
	     error_portal("nwfs:  sector total range error", 10);
             goto ScanNextDisk;
	  }

          // the first cylinder of all hard drives is reserved.
	  AdjustedSectors = (TotalSectors - SystemDisk[disk]->SectorsPerTrack);

          // scan for free slots and count the total free sectors
	  // for this drive.

	  for (freeSlots = AllocatedSectors = 0, i = 0; i < 4; i++)
	  {
             SlotMask[i] = 0;
             SlotLBA[i] = 0;
             SlotLength[i] = 0;
             FreeLBA[i] = 0;
             FreeLength[i] = 0;

	     if (SystemDisk[disk]->PartitionTable[i].SysFlag &&
                 SystemDisk[disk]->PartitionTable[i].nSectorsTotal &&
                 SystemDisk[disk]->PartitionTable[i].StartLBA)
	     {
		AllocatedSectors +=
                       SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
	     }
	     else
	     {
                FreeSlotIndex[freeSlots] = i;
		freeSlots++;
	     }
	  }

          // now sort the partition table into ascending order
          for (SlotCount = i = 0; i < 4; i++)
          {
             // look for the lowest starting lba
             for (SlotIndex = LBAStart = (ULONG) -1, j = 0; j < 4; j++)
             {
		if (SystemDisk[disk]->PartitionTable[j].SysFlag &&
                    !SlotMask[j])
                {
                   if (SystemDisk[disk]->PartitionTable[j].StartLBA < LBAStart)
                   {
                      LBAStart = SystemDisk[disk]->PartitionTable[j].StartLBA;
                      SlotIndex = j;
                   }
                }
             }

             if (SlotIndex < 4)
             {
                SlotMask[SlotIndex] = TRUE;
                SlotLBA[SlotCount] =
                    SystemDisk[disk]->PartitionTable[SlotIndex].StartLBA;
                SlotLength[SlotCount] =
                    SystemDisk[disk]->PartitionTable[SlotIndex].nSectorsTotal;
                SlotCount++;
             }
	  }

          // now determine how many contiguous free data areas are present
          // and what their sizes are in sectors.  we align the starting
          // LBA and size to cylinder boundries.

	  LBAStart = SystemDisk[disk]->SectorsPerTrack;
	  for (FreeCount = i = 0; i < SlotCount; i++)
	  {
	     if (LBAStart < SlotLBA[i])
	     {
#if (TRUE_NETWARE_MODE)
		// check if we are currently pointing to the
		// first track on this device.  If so, then
		// we begin our first defined partition as SPT.
		if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		{
		   LBAWork = LBAStart;
		   FreeLBA[FreeCount] = LBAWork;
		}
		else
		{
		   // round up to the next cylinder boundry
		   LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
		   FreeLBA[FreeCount] = LBAWork;
		}
#else
		// round up to the next cylinder boundry
		LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
		FreeLBA[FreeCount] = LBAWork;
#endif

		// adjust length to correspond to cylinder boundries
		LBAEnd = (((SlotLBA[i] +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		// if we rounded into the next partition, then adjust
		// the ending LBA to the beginning of the next partition.
		if (LBAEnd > SlotLBA[i])
		   LBAEnd = SlotLBA[i];

		// if we rounded off the end of the device, then adjust
		// the ending LBA to the total sector cout for the device.
		if (LBAEnd > TotalSectors)
		   LBAEnd = TotalSectors;

		Length = LBAEnd - LBAWork;
		FreeLength[FreeCount] = Length;

		FreeCount++;
	     }
	     LBAStart = (SlotLBA[i] + SlotLength[i]);
	  }

	  // determine how much free space exists less that claimed
	  // by the partition table after we finish scanning.  this
	  // case is the fall through case when the partition table
	  // is empty.

	  if (LBAStart < TotalSectors)
	  {
#if (TRUE_NETWARE_MODE)
	     // check if we are currently pointing to the
	     // first track on this device.  If so, then
	     // we begin our first defined partition as SPT.
	     if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
	     {
		LBAWork = LBAStart;
		FreeLBA[FreeCount] = LBAWork;
	     }
	     else
	     {
		// round up to the next cylinder boundry
		LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
		FreeLBA[FreeCount] = LBAWork;
	     }
#else
   	     // round up to the next cylinder boundry
	     LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
	     FreeLBA[FreeCount] = LBAWork;
#endif

	     // adjust length to correspond to cylinder boundries
	     LBAEnd = (((TotalSectors +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

	     // if we rounded off the end of the device, then adjust
	     // the ending LBA to the total sector cout for the device.
	     if (LBAEnd > TotalSectors)
		LBAEnd = TotalSectors;

	     Length = LBAEnd - LBAWork;
	     FreeLength[FreeCount] = Length;

	     FreeCount++;
	  }
	  else
	  if (LBAStart > TotalSectors)
	  {
	     // if LBAStart is greater than the TotalSectors for this
	     // device, then we have an error in the disk geometry
	     // dimensions and they are invalid, or we are running
	     // on MSDOS and it is reporting fewer cylinders than
	     // were reported to NetWare or Windows NT/2000.

             goto ScanNextDisk;
          }

	  // drive geometry range error or partition table data error,
          // abort the operation

	  if (AdjustedSectors < AllocatedSectors)
	  {
	     error_portal("nwfs:  drive geometry range error", 10);
             goto ScanNextDisk;
	  }

          // if there are no free sectors, then abort the operation
	  FreeSectors = (AdjustedSectors - AllocatedSectors);
	  if (!FreeSectors)
             goto ScanNextDisk;

          // if there were no free slots, then abort the operation
	  if (!freeSlots)
	     goto ScanNextDisk;

	  // make certain we have at least 10 MB of free space
	  // available or abort the operation.  don't allow the
          // creation of a Netware partition that is smaller than
          // 10 MB.

	  if (FreeSectors < 20480)
             goto ScanNextDisk;

          // now allocate all available free space on this device
          // to Netware.  if there are multiple contiguous
	  // free areas on the device, and free slots in
          // the partition table, then continue to allocate
          // Netware partitions until we have exhausted the
          // space on this device, or we have run out of
          // free slots in the partition table.

          for (i=0; (i < FreeCount) && freeSlots; i++)
          {
	     // get the first free slot available
             j = FreeSlotIndex[0];

	     SetPartitionTableValues(
                      &SystemDisk[disk]->PartitionTable[j],
		      NETWARE_386_ID,
		      FreeLBA[i],
		      (FreeLBA[i] + FreeLength[i]) - 1,
		      0,
		      SystemDisk[disk]->TracksPerCylinder,
		      SystemDisk[disk]->SectorsPerTrack);

	     // validate partition extants
	     if (ValidatePartitionExtants(disk))
	     {
		log_sprintf(displayBuffer,
			"partition table entry (%d) is invalid", (int)j);
		error_portal(displayBuffer, 10);
		goto ScanNextDisk;
	     }
	     SystemDisk[disk]->PartitionFlags[j] = TRUE;
	  }

	  retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	  if (!retCode)
	  {
	     log_sprintf(displayBuffer,
		     "FATAL:  disk-%d error reading boot sector", (int)disk);
	     error_portal(displayBuffer, 10);
	     goto ScanNextDisk;
	  }
	  NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	  // if there is no valid partition signature, then
	  // write a new master boot record

	  if (SystemDisk[disk]->partitionSignature != 0xAA55)
	  {
	     NWFSSet(Buffer, 0, 512);
	     NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
	     SystemDisk[disk]->partitionSignature = 0xAA55;
	     NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	  }

	  // copy the partition table and partition signature
	  NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

	  // write the partition table to sector 0
	  retCode = WriteDiskSectors(disk, 0, Buffer, 1, 1);
	  if (!retCode)
	  {
	     log_sprintf(displayBuffer,
		     "FATAL:  disk-%d error writing boot sector", (int)disk);
	     error_portal(displayBuffer, 10);
	     goto ScanNextDisk;
	  }

	  // if any of the partitions are being newly created, then
	  // read in their partition boot sectors and write out a valid
	  // Netware partition stamp.  We create partitions in
	  // Netware 5.x format.

	  for (i=0; i < 4; i++)
	  {
	     if ((SystemDisk[disk]->PartitionFlags[i]) &&
                 (SystemDisk[disk]->PartitionTable[i].SysFlag == 0x65) &&
                 (SystemDisk[disk]->PartitionTable[i].nSectorsTotal) &&
                 (SystemDisk[disk]->PartitionTable[i].StartLBA))
	     {
		retCode = ReadDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA,
			    Buffer, 1, 1);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d error reading part sector", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

		NWFSSet(Buffer, 0, IO_BLOCK_SIZE);
		ps = (PART_SIG *) Buffer;

                // create partition sector 0
		NWFSCopy(ps->NetwareSignature, NwPartSignature, 16);
		ps->PartitionType = TYPE_SIGNATURE;
		ps->PartitionSize = SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		ps->CreationDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA,
		      Buffer, 1, 1);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

		// write a netware boot sector to sector 1 on this
		// partition along with a complete copy of the
		// partition table for this drive.

		NWFSSet(Buffer, 0, 512);

                // create partition sector 1
		NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		SystemDisk[disk]->partitionSignature = 0xAA55;
		NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
		NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 1,
		      Buffer, 1, 1);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // zero out the partition hotfix and mirror tables

                // hotfix 0
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x20,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // mirror 0
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x21,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // hotfix 1
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x40,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // mirror 1
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x41,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			     "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // hotfix 2
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x60,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			  "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // mirror 2
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x61,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // hotfix 3
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x80,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}

                // mirror 3
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x81,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   log_sprintf(displayBuffer,
			   "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		   error_portal(displayBuffer, 10);
		   goto ScanNextDisk;
		}
	     }
	     SystemDisk[disk]->PartitionFlags[i] = 0;
	  }

	  ScanVolumes();
	  payload.payload_object_count = 0;
	  payload.payload_index = 0;
	  payload.payload_object_size_buffer_size = (sizeof(nwvp_rpartition_scan_info) << 20) + sizeof(rlist);
	  payload.payload_buffer = (BYTE *) &rlist[0];
	  do
	  {
	     nwvp_rpartition_scan(&payload);
	     for (j=0; j < payload.payload_object_count; j++)
	     {
		if ((rlist[j].lpart_handle == 0xFFFFFFFF) && ((rlist[j].partition_type == 0x65) || (rlist[j].partition_type ==0x77)))
		{
		   logical_capacity = (rlist[j].physical_block_count & 0xFFFFFF00) - 242;
		   raw_ids[0] = rlist[j].rpart_handle;
		   raw_ids[1] = 0xFFFFFFFF;
		   raw_ids[2] = 0xFFFFFFFF;
		   raw_ids[3] = 0xFFFFFFFF;
		   nwvp_lpartition_format(&vhandle, logical_capacity, &raw_ids[0]);
		}
	     }
	  } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
       }
       SyncDevice((ULONG)SystemDisk[disk]->PhysicalDiskHandle);

ScanNextDisk:;
    }

    if (message)
       close_message_portal(message);

    NWFSFree(Buffer);
    ScanVolumes();
    return 0;

}
ULONG menuFunctionPart(NWSCREEN *screen, ULONG value, BYTE *option, ULONG menu_index)
{
    register ULONG dmenu, message, dportal, ccode;
    BYTE displayBuffer[255];
    BYTE messageBuffer[255];
    BYTE workBuffer[10];
    ULONG disk, i, j, vhandle, raw_ids[4];
    ULONG TotalSectors, FreeSectors, AdjustedSectors;
    ULONG AllocatedSectors;
    ULONG LBAStart, LBAEnd, Length, freeSlots, targetSize;
    BYTE *Buffer;
    ULONG retCode;
    nwvp_rpartition_scan_info rlist[5];
    nwvp_payload payload;
    ULONG logical_capacity;
    register PART_SIG *ps;
    ULONG SlotCount;
    ULONG SlotIndex;
    ULONG SlotMask[4];
    ULONG SlotLBA[4];
    ULONG SlotLength[4];
    ULONG FreeCount;
    ULONG FreeLBA[4];
    ULONG FreeLength[4];
    ULONG FreeSlotIndex[4];
    ULONG LBAWork;
    ULONG MaxLBA;
    ULONG MaxLength;
    ULONG MaxSlot;
    ULONG line;

    Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Buffer)
    {
       error_portal("could not allocate partition workspace", 10);
       return 0;
    }
    NWFSSet(Buffer, 0, IO_BLOCK_SIZE);

    switch (value)
    {
       case 1:
	  disk = CurrentDisk;
	  if ((!SystemDisk[disk]) || (!SystemDisk[disk]->PhysicalDiskHandle))
	     break;

	  ccode = confirm_menu("  Write This Partition Table To Disk ?",
			       10, YELLOW | BGBLUE);
	  if (!ccode)
	     break;

	  log_sprintf(messageBuffer, "Updating BOOT Sector disk-(%d)", (int)disk);
	  message = create_message_portal(messageBuffer, 10, YELLOW | BGBLUE);

	  retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	  if (!retCode)
	  {
	     if (message)
		close_message_portal(message);
	     log_sprintf(displayBuffer, "FATAL:  disk-%d error reading boot sector", (int)disk);
	     error_portal(displayBuffer, 10);
	     break;
	  }
	  NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	  // if there is no valid partition signature, then
	  // write a new master boot record

	  if (SystemDisk[disk]->partitionSignature != 0xAA55)
	  {
	     NWFSSet(Buffer, 0, 512);
	     NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
	     SystemDisk[disk]->partitionSignature = 0xAA55;
	     NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	  }

	  // copy the partition table and partition signature
	  NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

	  // write the partition table to sector 0

	  retCode = WriteDiskSectors(disk, 0, Buffer, 1, 1);
	  if (!retCode)
	  {
	     if (message)
		close_message_portal(message);
	     log_sprintf(displayBuffer, "FATAL:  disk-%d error writing boot sector", (int)disk);
	     error_portal(displayBuffer, 10);
	     break;
	  }

	  if (message)
	     close_message_portal(message);

	  // if any of the partitions are being newly created, then
	  // read in their partition boot sectors and write out a valid
	  // Netware partition stamp.  We create partitions in
	  // Netware 3.x and 4.x/5.x formats.

	  for (i=0; i < 4; i++)
	  {
	     if ((SystemDisk[disk]->PartitionFlags[i]) &&
		 (SystemDisk[disk]->PartitionTable[i].SysFlag == 0x65) &&
		 (SystemDisk[disk]->PartitionTable[i].nSectorsTotal) &&
		 (SystemDisk[disk]->PartitionTable[i].StartLBA))
	     {
		if (SystemDisk[disk]->PartitionVersion[i] == NW4X_PARTITION)
		{
		   log_sprintf(messageBuffer,
			   "Formating NW4.x/5.x Partition (%d)-LBA-%08X size-%08X (%dMB)",
			   (int)disk,
			   (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			   (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			   (unsigned int)
			   (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal
			   * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			   / (LONGLONG)0x100000));
		   message = create_message_portal(messageBuffer, 10, YELLOW | BGBLUE);

		   retCode = ReadDiskSectors(disk,
			       SystemDisk[disk]->PartitionTable[i].StartLBA,
			       Buffer, 1, 1);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d error reading part sector", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   NWFSSet(Buffer, 0, IO_BLOCK_SIZE);
		   ps = (PART_SIG *) Buffer;

		   // create partition sector 0
		   NWFSCopy(ps->NetwareSignature, NwPartSignature, 16);
		   ps->PartitionType = TYPE_SIGNATURE;
		   ps->PartitionSize = SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		   ps->CreationDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA,
			    Buffer, 1, 1);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // write a Netware boot sector to sector 1 on this
		   // partition along with a complete copy of the
		   // partition table for this drive.

		   NWFSSet(Buffer, 0, 512);

		   // create partition sector 1
		   NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		   SystemDisk[disk]->partitionSignature = 0xAA55;
		   NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
		   NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 1,
			    Buffer, 1, 1);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // zero out the partition hotfix and mirror tables
		   // hotfix 0

		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x20,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 0
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x21,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // hotfix 1
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x40,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 1
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x41,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // hotfix 2
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x60,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 2
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x61,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // hotfix 3
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x80,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 3
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x81,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }
		   if (message)
		      close_message_portal(message);
		}
		else
		if (SystemDisk[disk]->PartitionVersion[i] == NW3X_PARTITION)
		{
		   log_sprintf(messageBuffer,
			   "Formating NW3.x Partition (%d)-LBA-%08X size-%08X (%dMB)",
			   (int)disk,
			   (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			   (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			   (unsigned int)
			    (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal
			    * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			    / (LONGLONG)0x100000));
		   message = create_message_portal(messageBuffer, 10, YELLOW | BGBLUE);

		   retCode = ReadDiskSectors(disk,
			       SystemDisk[disk]->PartitionTable[i].StartLBA,
			       Buffer, 1, 1);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d error reading part sector", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // zero 3.x partition sector 0
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA,
			    ZeroBuffer, 1, 1);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // zero 3.x partition sector 1
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 1,
			    ZeroBuffer, 1, 1);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // zero out the partition hotfix and mirror tables

		   // hotfix 0
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x20,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 0
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x21,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // hotfix 1
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x40,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 1
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x41,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // hotfix 2
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x60,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 2
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x61,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // hotfix 3
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x80,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   // mirror 3
		   retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x81,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		   if (!retCode)
		   {
		      if (message)
			 close_message_portal(message);
		      log_sprintf(displayBuffer, "FATAL:  disk-%d hotfix/mirror init error", (int)disk);
		      error_portal(displayBuffer, 10);
		      break;
		   }
		   if (message)
		      close_message_portal(message);
		}
		else
		{
		   log_sprintf(displayBuffer, "FATAL:  disk-%d could not determine partition version", (int)disk);
		   error_portal(displayBuffer, 10);
		}
	     }
	     SystemDisk[disk]->PartitionFlags[i] = 0;
	  }

	  ScanVolumes();
	  payload.payload_object_count = 0;
	  payload.payload_index = 0;
	  payload.payload_object_size_buffer_size = (sizeof(nwvp_rpartition_scan_info) << 20) + sizeof(rlist);
	  payload.payload_buffer = (BYTE *) &rlist[0];
	  do
	  {
	     nwvp_rpartition_scan(&payload);
	     for (j=0; j< payload.payload_object_count; j++)
	     {
		if ((rlist[j].lpart_handle == 0xFFFFFFFF) && ((rlist[j].partition_type == 0x65) || (rlist[j].partition_type == 0x77)))
		{
		   logical_capacity = (rlist[j].physical_block_count & 0xFFFFFF00) - 242;
		   raw_ids[0] = rlist[j].rpart_handle;
		   raw_ids[1] = 0xFFFFFFFF;
		   raw_ids[2] = 0xFFFFFFFF;
		   raw_ids[3] = 0xFFFFFFFF;
		   nwvp_lpartition_format(&vhandle, logical_capacity, &raw_ids[0]);
		}
	     }
	  } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

	  SyncDevice((ULONG)SystemDisk[disk]->PhysicalDiskHandle);
	  break;

       case 2:
	  dmenu = make_menu(&ConsoleScreen,
			  "  Select Disk Device ",
			  8,
			  28,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  MAX_DISKS);

	  if (!dmenu)
	     break;

	  for (i=0; i < MAX_DISKS; i++)
	  {
	     if (SystemDisk[i] && SystemDisk[i]->PhysicalDiskHandle)
             {
	        add_item_to_menu(dmenu, disk_name_array[i], i);
             }
	  }

	  disk = activate_menu(dmenu);

	  if (dmenu)
	     free_menu(dmenu);

	  if (disk == (ULONG) -1)
	     break;

	  if ((disk < MAX_DISKS) && (SystemDisk[disk]) &&
	      (SystemDisk[disk]->PhysicalDiskHandle)) 
	  {
	     CurrentDisk = disk;
	     if (pportal)
	     {
		deactivate_static_portal(pportal);
		free_portal(pportal);
	     }

#if (LINUX_UTIL)
             sprintf(pportal_header, "Disk-#%d %s (%02X:%02X) Sectors-0x%X Size-%.0f bytes",
	       (int)disk,
               PhysicalDiskNames[disk],
	       (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle & 0xFF),
	       (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle >> 8),
	       (unsigned int)((SystemDisk[disk]->Cylinders) *
			       SystemDisk[disk]->TracksPerCylinder *
			       SystemDisk[disk]->SectorsPerTrack),
	       (double)SystemDisk[disk]->driveSize);

#else	    
             sprintf(pportal_header, "Disk-#%d Sectors-0x%X Size-%.0f bytes",
	       (int)disk,
	       (unsigned int)((SystemDisk[disk]->Cylinders) *
			       SystemDisk[disk]->TracksPerCylinder *
			       SystemDisk[disk]->SectorsPerTrack),
	       (double)SystemDisk[disk]->driveSize);
#endif

	     pportal = make_portal(&ConsoleScreen,
			       pportal_header,
			       0,
			       3,
			       0,
			       13,
			       ConsoleScreen.nCols - 1,
			       10,
			       BORDER_SINGLE,
			       YELLOW | BGBLUE,
			       YELLOW | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       0,
			       0,
			       0,
			       TRUE);
	     if (!pportal)
		break;

	     activate_static_portal(pportal);
	  }
	  break;

       case 3:
	  disk = CurrentDisk;
	  if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	  {
	     TotalSectors = (ULONG)((SystemDisk[disk]->Cylinders) *
				       SystemDisk[disk]->TracksPerCylinder *
				       SystemDisk[disk]->SectorsPerTrack);

	     if (TotalSectors < SystemDisk[disk]->SectorsPerTrack)
	     {
		error_portal("nwfs:  sector total range error", 10);
		break;
	     }

	     // assume the first cylinder of all hard drives is reserved.
	     AdjustedSectors = (TotalSectors - SystemDisk[disk]->SectorsPerTrack);

	     // scan for free slots and count the total free sectors
	     // for this drive.

	     for (freeSlots = AllocatedSectors = 0, i = 0; i < 4; i++)
	     {
		   SlotMask[i] = 0;
		   SlotLBA[i] = 0;
		   SlotLength[i] = 0;
		   FreeLBA[i] = 0;
		   FreeLength[i] = 0;

		   if (SystemDisk[disk]->PartitionTable[i].SysFlag &&
		       SystemDisk[disk]->PartitionTable[i].nSectorsTotal &&
		       SystemDisk[disk]->PartitionTable[i].StartLBA)
		   {
		      AllocatedSectors +=
			   SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		   }
		   else
		   {
		      FreeSlotIndex[freeSlots] = i;
		      freeSlots++;
		   }
		}

                // now sort the partition table into ascending order
		for (SlotCount = i = 0; i < 4; i++)
                {
                   // look for the lowest starting lba
                   for (SlotIndex = LBAStart = (ULONG) -1, j = 0; j < 4; j++)
                   {
	              if (SystemDisk[disk]->PartitionTable[j].SysFlag &&
                          !SlotMask[j])
		      {
			 if (SystemDisk[disk]->PartitionTable[j].StartLBA <
                             LBAStart)
			 {
                            LBAStart = SystemDisk[disk]->PartitionTable[j].StartLBA;
			    SlotIndex = j;
                         }
                      }
                   }

		   if (SlotIndex < 4)
                   {
                      SlotMask[SlotIndex] = TRUE;
		      SlotLBA[SlotCount] =
                         SystemDisk[disk]->PartitionTable[SlotIndex].StartLBA;
                      SlotLength[SlotCount] =
                         SystemDisk[disk]->PartitionTable[SlotIndex].nSectorsTotal;
                      SlotCount++;
                   }
                }

		// now determine how many contiguous free data areas are
                // present and what their sizes are in sectors.  we align
		// the starting LBA and size to cylinder boundries.

		LBAStart = SystemDisk[disk]->SectorsPerTrack;
		for (FreeCount = i = 0; i < SlotCount; i++)
		{
		   if (LBAStart < SlotLBA[i])
		   {
		      // check if we are currently pointing to the
		      // first track on this device.  If so, then
		      // we begin our first defined partition on the
		      // first track of the device.

		      if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		      {
			 LBAWork = LBAStart;
			 FreeLBA[FreeCount] = LBAWork;
		      }
		      else
		      {
			 // round up to the next cylinder boundry
			 LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
			 FreeLBA[FreeCount] = LBAWork;
		      }

		      // adjust length to correspond to cylinder boundries
		      LBAEnd = (((SlotLBA[i] +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		      // if we rounded into the next partition, then adjust
		      // the ending LBA to the beginning of the next partition.
		      if (LBAEnd > SlotLBA[i])
			 LBAEnd = SlotLBA[i];

		      // if we rounded off the end of the device, then adjust
		      // the ending LBA to the total sector cout for the device.
		      if (LBAEnd > TotalSectors)
			 LBAEnd = TotalSectors;

		      Length = LBAEnd - LBAWork;
		      FreeLength[FreeCount] = Length;

		      FreeCount++;
                   }
                   LBAStart = (SlotLBA[i] + SlotLength[i]);
                }

                // determine how much free space exists less that claimed
		// by the partition table after we finish scanning.  this
                // case is the fall through case when the partition table
		// is empty.

		if (LBAStart < TotalSectors)
		{
#if (TRUE_NETWARE_MODE)
		   // check if we are currently pointing to the
		   // first track on this device.  If so, then
		   // we begin our first defined partition as SPT.
		   if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		   {
		      LBAWork = LBAStart;
		      FreeLBA[FreeCount] = LBAWork;
		   }
		   else
		   {
		      // round up to the next cylinder boundry
		      LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		      FreeLBA[FreeCount] = LBAWork;
		   }
#else
		   // round up to the next cylinder boundry
		   LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		   FreeLBA[FreeCount] = LBAWork;
#endif

		   // adjust length to correspond to cylinder boundries
		   LBAEnd = (((TotalSectors +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		   // if we rounded off the end of the device, then adjust
		   // the ending LBA to the total sector cout for the device.
		   if (LBAEnd > TotalSectors)
		      LBAEnd = TotalSectors;

		   Length = LBAEnd - LBAWork;
		   FreeLength[FreeCount] = Length;

		   FreeCount++;
		}
		else
		if (LBAStart > TotalSectors)
		{
		   // if LBAStart is greater than the TotalSectors for this
		   // device, then we have an error in the disk geometry
		   // dimensions and they are invalid, or we are running
		   // on MSDOS and it is reporting fewer cylinders than
		   // were reported to NetWare or Windows NT/2000.

		   error_portal("drive reports fewer cylinders than partition table", 10);
		}

		// drive geometry range error or partition table data error,
                // abort the operation

	        if (AdjustedSectors < AllocatedSectors)
		{
		   error_portal("***  drive geometry range error ***", 10);
		   break;
		}

		// if there are no free sectors, then abort the operation
		FreeSectors = (AdjustedSectors - AllocatedSectors);

		if (!FreeSectors)
		{
		   error_portal("*** No logical space left on device ***", 10);
		   break;
                }

		// if there were no free slots, then abort the operation
	        if (!freeSlots)
		{
		   error_portal("*** No partition table entries available on device ***", 10);
		   break;
                }

	        // make certain we have at least 10 MB of free space
		// available or abort the operation.  don't allow the
		// creation of a Netware partition that is smaller than
                // 10 MB.

	        if (FreeSectors < 20480)
		{
		   error_portal("*** you must have at least 10 MB of free space ***", 10);
		   break;
                }

		// determine which entry has the single largest area of
                // contiguous setors for this device.

		for (MaxSlot = MaxLBA = MaxLength = i = 0;
                     i < FreeCount; i++)
		{
                   if (FreeLength[i] > MaxLength)
                   {
                      MaxLBA = FreeLBA[i];
		      MaxLength = FreeLength[i];
		   }
		}

		sprintf(displayBuffer,
		       "  Assign all free space to NW3.x? (%uMB) ",
			(unsigned int)(((LONGLONG)SystemDisk[disk]->BytesPerSector *
			(LONGLONG)MaxLength) / (LONGLONG)0x100000));
		retCode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);

		if (retCode)
		{
		   // get the first free slot available
		   j = FreeSlotIndex[0];

		   SetPartitionTableValues(
			 &SystemDisk[disk]->PartitionTable[j],
			 NETWARE_386_ID,
			 MaxLBA,
		         (MaxLBA + MaxLength) - 1,
			 0,
		         SystemDisk[disk]->TracksPerCylinder,
			 SystemDisk[disk]->SectorsPerTrack);

                   // validate partition extants
		   if (ValidatePartitionExtants(disk))
		   {
		      log_sprintf(displayBuffer,
			   "partition table entry (%d) is invalid", (int)j);
		      error_portal(displayBuffer, 10);
		      break;
	           }

	           SystemDisk[disk]->PartitionFlags[j] = TRUE;
		   SystemDisk[disk]->PartitionVersion[j] = NW3X_PARTITION;

		   sprintf(displayBuffer,
			     "NW3.x Partition %d:%d LBA-%08X size-%08X (%dMB) Created",
			     (int)disk, (int)j,
			     (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
			     (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
			     (unsigned int)
			       (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
			       * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			       / (LONGLONG)0x100000));
		   message_portal(displayBuffer, 10, YELLOW | BGBLUE, TRUE);
		   break;
		}

		dportal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       10,
		       5,
		       14,
		       75,
		       3,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

		if (!dportal)
		   break;

		activate_static_portal(dportal);

                NWFSSet(workBuffer, 0, sizeof(workBuffer));

		sprintf(displayBuffer,
			"Enter NW3.x Partition Size (up to %uMB): ",
		       (unsigned int)
			 (((LONGLONG)SystemDisk[disk]->BytesPerSector *
			   (LONGLONG)MaxLength) /
			   (LONGLONG)0x100000));

		ccode = add_field_to_portal(dportal, 1, 2, WHITE | BGBLUE,
			 displayBuffer, strlen(displayBuffer),
			 workBuffer, sizeof(workBuffer),
			 0, 0, 0, 0, FIELD_ENTRY);
		if (ccode)
		{
		   error_portal("****  error:  could not allocate memory ****", 10);
		   if (dportal)
		   {
			  deactivate_static_portal(dportal);
		      free_portal(dportal);
		   }
		   break;
		}

		ccode = input_portal_fields(dportal);
		if (ccode)
		{
		   if (dportal)
		   {
		      deactivate_static_portal(dportal);
		      free_portal(dportal);
		   }
		   break;
		}

		if (dportal)
		{
		   deactivate_static_portal(dportal);
		   free_portal(dportal);
		}

		targetSize = atol(workBuffer);
		if (!targetSize)
		   break;

		if ((targetSize >
		     (ULONG)(((LONGLONG)SystemDisk[disk]->BytesPerSector *
			      (LONGLONG)MaxLength) / (LONGLONG)0x100000))
		     || (targetSize < 10))
		   break;

		// convert bytes to sectors
		MaxLength = (ULONG)(((LONGLONG)targetSize *
				(LONGLONG)0x100000) /
				(LONGLONG)SystemDisk[disk]->BytesPerSector);

		// cylinder align the requested size
		MaxLength = (((MaxLength +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));

		// get the first free slot available
		j = FreeSlotIndex[0];

		SetPartitionTableValues(
			       &SystemDisk[disk]->PartitionTable[j],
			       NETWARE_386_ID,
			       MaxLBA,
			       (MaxLBA + MaxLength) - 1,
			       0,
			       SystemDisk[disk]->TracksPerCylinder,
			       SystemDisk[disk]->SectorsPerTrack);

		SystemDisk[disk]->PartitionFlags[j] = TRUE;
		SystemDisk[disk]->PartitionVersion[j] = NW3X_PARTITION;

		sprintf(displayBuffer,
			    "NW3.x Partition %d:%d LBA-%08X size-%08X (%dMB) Created",
			    (int)disk, (int)j,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
			    (unsigned int)
			      (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
			      * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			      / (LONGLONG)0x100000));
		message_portal(displayBuffer, 10, YELLOW | BGBLUE, TRUE);
	  }
	  break;

       case 4:
	  disk = CurrentDisk;
	  if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	  {
		TotalSectors = (ULONG)((SystemDisk[disk]->Cylinders) *
				       SystemDisk[disk]->TracksPerCylinder *
				       SystemDisk[disk]->SectorsPerTrack);

		if (TotalSectors < SystemDisk[disk]->SectorsPerTrack)
		{
		   error_portal("nwfs:  sector total range error", 10);
		   break;
	        }

		// assume the first cylinder of all hard drives is reserved.
	        AdjustedSectors = (TotalSectors - SystemDisk[disk]->SectorsPerTrack);

		// scan for free slots and count the total free sectors
                // for this drive.

	        for (freeSlots = AllocatedSectors = 0, i = 0; i < 4; i++)
	        {
		   SlotMask[i] = 0;
                   SlotLBA[i] = 0;
                   SlotLength[i] = 0;
		   FreeLBA[i] = 0;
                   FreeLength[i] = 0;

		   if (SystemDisk[disk]->PartitionTable[i].SysFlag &&
                       SystemDisk[disk]->PartitionTable[i].nSectorsTotal &&
                       SystemDisk[disk]->PartitionTable[i].StartLBA)
		   {
		      AllocatedSectors +=
			   SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		   }
		   else
		   {
		      FreeSlotIndex[freeSlots] = i;
		      freeSlots++;
		   }
	        }

                // now sort the partition table into ascending order
                for (SlotCount = i = 0; i < 4; i++)
		{
                   // look for the lowest starting lba
                   for (SlotIndex = LBAStart = (ULONG) -1, j = 0; j < 4; j++)
                   {
		      if (SystemDisk[disk]->PartitionTable[j].SysFlag &&
			  !SlotMask[j])
		      {
			 if (SystemDisk[disk]->PartitionTable[j].StartLBA <
                             LBAStart)
                         {
			    LBAStart = SystemDisk[disk]->PartitionTable[j].StartLBA;
                            SlotIndex = j;
                         }
		      }
                   }

                   if (SlotIndex < 4)
                   {
                      SlotMask[SlotIndex] = TRUE;
                      SlotLBA[SlotCount] =
			 SystemDisk[disk]->PartitionTable[SlotIndex].StartLBA;
                      SlotLength[SlotCount] =
                         SystemDisk[disk]->PartitionTable[SlotIndex].nSectorsTotal;
                      SlotCount++;
		   }
		}

		// now determine how many contiguous free data areas are
                // present and what their sizes are in sectors.  we align
                // the starting LBA and size to cylinder boundries.

	        LBAStart = SystemDisk[disk]->SectorsPerTrack;
                for (FreeCount = i = 0; i < SlotCount; i++)
		{
                   if (LBAStart < SlotLBA[i])
		   {
		      // check if we are currently pointing to the
		      // first track on this device.  If so, then
		      // we begin our first defined partition on
		      // the first track of this device.

		      if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		      {
			 LBAWork = LBAStart;
			 FreeLBA[FreeCount] = LBAWork;
		      }
		      else
		      {
			 // round up to the next cylinder boundry
			 LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
			 FreeLBA[FreeCount] = LBAWork;
		      }

		      // adjust length to correspond to cylinder boundries
		      LBAEnd = (((SlotLBA[i] +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		      // if we rounded into the next partition, then adjust
		      // the ending LBA to the beginning of the next partition.
		      if (LBAEnd > SlotLBA[i])
			 LBAEnd = SlotLBA[i];

		      // if we rounded off the end of the device, then adjust
		      // the ending LBA to the total sector cout for the device.
		      if (LBAEnd > TotalSectors)
			 LBAEnd = TotalSectors;

		      Length = LBAEnd - LBAWork;
		      FreeLength[FreeCount] = Length;
		      FreeCount++;
		   }
		   LBAStart = (SlotLBA[i] + SlotLength[i]);
		}

		// determine how much free space exists less that claimed
		// by the partition table after we finish scanning.  this
		// case is the fall through case when the partition table
                // is empty.

		if (LBAStart < TotalSectors)
		{
#if (TRUE_NETWARE_MODE)
		   // check if we are currently pointing to the
		   // first track on this device.  If so, then
		   // we begin our first defined partition as SPT.
		   if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		   {
		      LBAWork = LBAStart;
		      FreeLBA[FreeCount] = LBAWork;
		   }
		   else
		   {
		      // round up to the next cylinder boundry
		      LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		      FreeLBA[FreeCount] = LBAWork;
		   }
#else
		   // round up to the next cylinder boundry
		   LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		   FreeLBA[FreeCount] = LBAWork;
#endif

		   // adjust length to correspond to cylinder boundries
		   LBAEnd = (((TotalSectors +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		   // if we rounded off the end of the device, then adjust
		   // the ending LBA to the total sector cout for the device.
		   if (LBAEnd > TotalSectors)
		      LBAEnd = TotalSectors;

		   Length = LBAEnd - LBAWork;
		   FreeLength[FreeCount] = Length;
		   FreeCount++;
		}
		else
		if (LBAStart > TotalSectors)
		{
		   // if LBAStart is greater than the TotalSectors for this
		   // device, then we have an error in the disk geometry
		   // dimensions and they are invalid.

		   error_portal("drive reports fewer cylinders than partition table", 10);
		   break;
		}

		// drive geometry range error or partition table data error,
		// abort the operation

		if (AdjustedSectors < AllocatedSectors)
		{
		   error_portal("***  drive geometry range error ***", 10);
		   break;
	        }

                // if there are no free sectors, then abort the operation
	        FreeSectors = (AdjustedSectors - AllocatedSectors);

		if (!FreeSectors)
                {
		   error_portal("*** No logical space left on device ***", 10);
		   break;
		}

		// if there were no free slots, then abort the operation
		if (!freeSlots)
		{
		   error_portal("*** No partition table entries available on device ***", 10);
		   break;
                }

		// make certain we have at least 10 MB of free space
	        // available or abort the operation.  don't allow the
		// creation of a Netware partition that is smaller than
                // 10 MB.

	        if (FreeSectors < 20480)
                {
		   error_portal("*** you must have at least 10 MB of free space ***", 10);
		   break;
		}

		// determine which entry has the single largest area of
		// contiguous setors for this device.

		for (MaxSlot = MaxLBA = MaxLength = i = 0;
		     i < FreeCount; i++)
		{
		   if (FreeLength[i] > MaxLength)
                   {
		      MaxLBA = FreeLBA[i];
                      MaxLength = FreeLength[i];
		   }
                }

		sprintf(displayBuffer,
		       "  Assign all free space to NW4.x/5.x? (%uMB) ",
			(unsigned int)(((LONGLONG)SystemDisk[disk]->BytesPerSector *
			(LONGLONG)MaxLength) / (LONGLONG)0x100000));
		retCode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);

		if (retCode)
		{
                   // get the first free slot available
                   j = FreeSlotIndex[0];

	           SetPartitionTableValues(
			 &SystemDisk[disk]->PartitionTable[j],
		         NETWARE_386_ID,
		         MaxLBA,
			 (MaxLBA + MaxLength) - 1,
			 0,
		         SystemDisk[disk]->TracksPerCylinder,
		         SystemDisk[disk]->SectorsPerTrack);

                   // validate partition extants
	           if (ValidatePartitionExtants(disk))
	           {
		      log_sprintf(displayBuffer, "partition table entry (%d) is invalid", (int)j);
		      error_portal(displayBuffer, 10);
		      break;
		   }

		   SystemDisk[disk]->PartitionFlags[j] = TRUE;
		   SystemDisk[disk]->PartitionVersion[j] = NW4X_PARTITION;

		   sprintf(displayBuffer,
			   "NW4.x/5.x Partition %d:%d LBA-%08X size-%08X (%dMB) Created",
			    (int)disk, (int)j,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
			    (unsigned int)
			       (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
			       * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			       / (LONGLONG)0x100000));
		   message_portal(displayBuffer, 10, YELLOW | BGBLUE, TRUE);
		   break;
		}

		dportal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       10,
		       5,
		       14,
		       75,
		       3,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

		if (!dportal)
		   break;

		activate_static_portal(dportal);

                NWFSSet(workBuffer, 0, sizeof(workBuffer));

		sprintf(displayBuffer,
			"Enter NW4.x/5.x Partition Size (up to %uMB): ",
		       (unsigned int)
			 (((LONGLONG)SystemDisk[disk]->BytesPerSector *
			   (LONGLONG)MaxLength) /
			   (LONGLONG)0x100000));

		ccode = add_field_to_portal(dportal, 1, 2, WHITE | BGBLUE,
			 displayBuffer, strlen(displayBuffer),
			 workBuffer, sizeof(workBuffer),
			 0, 0, 0, 0, FIELD_ENTRY);
		if (ccode)
		{
		   error_portal("****  error:  could not allocate memory ****", 10);
		   if (dportal)
		   {
		      deactivate_static_portal(dportal);
		      free_portal(dportal);
		   }
		   break;
		}

		ccode = input_portal_fields(dportal);
		if (ccode)
		{
		   if (dportal)
		   {
		      deactivate_static_portal(dportal);
		      free_portal(dportal);
		   }
		   break;
		}

		if (dportal)
		{
		   deactivate_static_portal(dportal);
		   free_portal(dportal);
		}


		targetSize = atol(workBuffer);
		if (!targetSize)
		   break;

		if ((targetSize >
		     (ULONG)(((LONGLONG)SystemDisk[disk]->BytesPerSector *
			      (LONGLONG)MaxLength) / (LONGLONG)0x100000))
		       && (targetSize < 10))
		   break;

		// convert bytes to sectors
		MaxLength = (ULONG)(((LONGLONG)targetSize *
				(LONGLONG)0x100000) /
				(LONGLONG)SystemDisk[disk]->BytesPerSector);

		// cylinder align the requested size
		MaxLength = (((MaxLength +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		// get the first free slot available
		j = FreeSlotIndex[0];

		SetPartitionTableValues(
			       &SystemDisk[disk]->PartitionTable[j],
			       NETWARE_386_ID,
			       MaxLBA,
			       (MaxLBA + MaxLength) - 1,
			       0,
			       SystemDisk[disk]->TracksPerCylinder,
			       SystemDisk[disk]->SectorsPerTrack);

		SystemDisk[disk]->PartitionFlags[j] = TRUE;
		SystemDisk[disk]->PartitionVersion[j] = NW4X_PARTITION;

		sprintf(displayBuffer,
			   "NW4.x/5.x Partition %d:%d LBA-%08X size-%08X (%dMB) Created",
			    (int)disk, (int)j,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
			    (unsigned int)
			      (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
			      * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			      / (LONGLONG)0x100000));
		message_portal(displayBuffer, 10, YELLOW | BGBLUE, TRUE);
	  }
	  break;

       case 5:
	  disk = CurrentDisk;
	  if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	  {
	     ULONG delmenu, pnum;
	     BYTE partDescription[4][100];

	     delmenu = make_menu(&ConsoleScreen,
			  "   Select a Partition To Make Active ",
			  8,
			  19,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  0);

	     if (!delmenu)
		break;

	     for (j=0; j < 4; j++)
	     {
		if (SystemDisk[disk]->PartitionTable[j].nSectorsTotal)
		{
		   if (SystemDisk[disk]->PartitionTable[j].SysFlag == NETWARE_386_ID)
		      sprintf(partDescription[j], "#%d %s %s %s",
			   (int)(j + 1),
			    SystemDisk[disk]->PartitionTable[j].fBootable ? "A" : " ",
			    get_partition_type(SystemDisk[disk]->PartitionTable[j].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[j] ? "3.x"
			     : "4.x/5.x"));
		   else
		      sprintf(partDescription[j], "#%d %s %s",
			   (int)(j + 1),
			    SystemDisk[disk]->PartitionTable[j].fBootable ? "A" : " ",
			    get_partition_type(SystemDisk[disk]->PartitionTable[j].SysFlag));
		   add_item_to_menu(delmenu, partDescription[j], j);
		}
	     }

	     pnum = activate_menu(delmenu);

	     if (delmenu)
		free_menu(delmenu);

	     if (pnum == (ULONG) -1)
		break;

	     for (j=0; j < 4; j++)
	     {
		if (SystemDisk[disk]->PartitionTable[j].nSectorsTotal)
		   SystemDisk[disk]->PartitionTable[j].fBootable = 0;
	     }
	     SystemDisk[disk]->PartitionTable[pnum].fBootable = 1;
	  }
	  break;

       case 6:
	  disk = CurrentDisk;
	  if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	  {
	     ULONG delmenu, pnum;
	     BYTE partDescription[4][100];

	     delmenu = make_menu(&ConsoleScreen,
			  "   Select a Partition To Delete ",
			  8,
			  22,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  0);

	     if (!delmenu)
		break;

	     for (j=0; j < 4; j++)
	     {
		if (SystemDisk[disk]->PartitionTable[j].nSectorsTotal)
		{
		   if (SystemDisk[disk]->PartitionTable[j].SysFlag == NETWARE_386_ID)
		      sprintf(partDescription[j], "#%d %s %s %s",
			   (int)(j + 1),
			    SystemDisk[disk]->PartitionTable[j].fBootable ? "A" : " ",
			    get_partition_type(SystemDisk[disk]->PartitionTable[j].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[j] ? "3.x"
			     : "4.x/5.x"));
		   else
		      sprintf(partDescription[j], "#%d %s %s",
			   (int)(j + 1),
			    SystemDisk[disk]->PartitionTable[j].fBootable ? "A" : " ",
			    get_partition_type(SystemDisk[disk]->PartitionTable[j].SysFlag));
		   add_item_to_menu(delmenu, partDescription[j], j);
		}
	     }

	     pnum = activate_menu(delmenu);

	     if (delmenu)
		free_menu(delmenu);

	     if (pnum == (ULONG) -1)
		break;

	     sprintf(displayBuffer, "  Delete Partition %d ?", (int) (pnum + 1));
	     ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	     if (!ccode)
		break;

	     NWFSSet(&SystemDisk[disk]->PartitionTable[pnum], 0,
		    sizeof(struct PartitionTableEntry));
	  }
	  break;

       case 7:
	  disk = CurrentDisk;
	  if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	  {
	     ccode = confirm_menu("  Are You Certain You Want To Erase This Drive ? ",
				  10, YELLOW | BGBLUE);
	     if (!ccode)
		break;

	     retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
		log_sprintf(displayBuffer, "FATAL:  disk-%d error reading boot sector", (int)disk);
		error_portal(displayBuffer, 10);
		break;
	     }

	     NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	     // if there is no valid partition signature, then
	     // write a new master boot record.

	     if (SystemDisk[disk]->partitionSignature != 0xAA55)
	     {
		NWFSSet(Buffer, 0, 512);
		NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		SystemDisk[disk]->partitionSignature = 0xAA55;
		NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	     }

	     // zero the partition table and partition signature
	     NWFSSet(&SystemDisk[disk]->PartitionTable[0].fBootable, 0, 64);
	     NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);
	  }
	  break;

       case 8:
	  ccode = confirm_menu("  Allocate All Free Space To NW4.x/5.x ?",
			       10, YELLOW | BGBLUE);
	  if (!ccode)
	     break;

	  NWConfigAllocateUnpartitionedSpace();
	  break;

       default:
	  break;
    }
    NWFSFree(Buffer);

    disk = CurrentDisk;
    line = 0;
    if (ValidatePartitionExtants(disk))
    {
       log_sprintf(displayBuffer, "disk-(%d) partition table is invalid", (int)disk);
       write_portal(pportal, displayBuffer, line++, 0, YELLOW | BGBLUE);
    }

    if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
    {
#if (LINUX_UTIL)
             sprintf(pportal_header, "Disk-#%d %s (%02X:%02X) Sectors-0x%X Size-%.0f bytes",
	       (int)disk,
               PhysicalDiskNames[disk],
	       (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle & 0xFF),
	       (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle >> 8),
	       (unsigned int)((SystemDisk[disk]->Cylinders) *
			       SystemDisk[disk]->TracksPerCylinder *
			       SystemDisk[disk]->SectorsPerTrack),
	       (double)SystemDisk[disk]->driveSize);

#else	    
             sprintf(pportal_header, "Disk-#%d Sectors-0x%X Size-%.0f bytes",
	       (int)disk,
	       (unsigned int)((SystemDisk[disk]->Cylinders) *
			       SystemDisk[disk]->TracksPerCylinder *
			       SystemDisk[disk]->SectorsPerTrack),
	       (double)SystemDisk[disk]->driveSize);
#endif
       write_portal_header1(pportal, pportal_header, YELLOW | BGBLUE);
	     
       sprintf(displayBuffer, "     Cylinders       : %12u  ", (unsigned int)SystemDisk[disk]->Cylinders);
       write_portal(pportal, displayBuffer, line, 0, YELLOW | BGBLUE);

       sprintf(displayBuffer, "Tracks/Cylinder : %12u", (unsigned int)SystemDisk[disk]->TracksPerCylinder);
       write_portal(pportal, displayBuffer, line++, 39, YELLOW | BGBLUE);

       sprintf(displayBuffer, "     SectorsPerTrack : %12u  ", (unsigned int)SystemDisk[disk]->SectorsPerTrack);
       write_portal(pportal, displayBuffer, line, 0, YELLOW | BGBLUE);

       sprintf(displayBuffer, "BytesPerSector  : %12u", (unsigned int)SystemDisk[disk]->BytesPerSector);
       write_portal(pportal, displayBuffer, line, 39, YELLOW | BGBLUE);

       line += 2;
       for (i=0; i < 4; i++)
       {
	  if ((SystemDisk[disk]->PartitionTable[i].SysFlag) &&
	      (SystemDisk[disk]->PartitionTable[i].nSectorsTotal))
	  {
	     if (SystemDisk[disk]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	     {
		sprintf(displayBuffer,
			"#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB  %s %s",
			    (int)(i + 1),
			    SystemDisk[disk]->PartitionTable[i].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[i]
			     ? "3.x"
			     : "4.x/5.x"));
		write_portal_cleol(pportal, " ", line, 2, YELLOW | BGBLUE);
		write_portal(pportal, displayBuffer, line++, 2, YELLOW | BGBLUE);
	     }
	     else
	     {
		sprintf(displayBuffer,
			"#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB  %s",
			    (int)(i + 1),
			    SystemDisk[disk]->PartitionTable[i].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag));
		write_portal_cleol(pportal, " ", line, 2, YELLOW | BGBLUE);
		write_portal(pportal, displayBuffer, line++, 2, YELLOW | BGBLUE);
	     }
	  }
	  else
	  {
	     sprintf(displayBuffer, "#%d  ------------- Unused ------------- ", (int)(i + 1));
	     write_portal_cleol(pportal, " ", line, 2, YELLOW | BGBLUE);
	     write_portal(pportal, displayBuffer, line++, 2, YELLOW | BGBLUE);
	  }
       }
    }
    update_static_portal(pportal);
    return 0;
}

ULONG menuKeyPart(NWSCREEN *screen, ULONG key, ULONG index)
{
    ULONG help;
    BYTE displayBuffer[255];
    register ULONG i;

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  help = make_portal(&ConsoleScreen,
		       " Help for Partition Management ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       256,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(PartitionHelp) / sizeof (BYTE *); i++)
	     write_portal_cleol(help, PartitionHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F1-Help  F3-Menu");
#else
	  sprintf(displayBuffer, "  F1-Help  ESC-Menu");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;
    }
    return 0;
}

void NWConfigEditPartitions(void)
{
    ULONG pmenu, ccode, i, disk, line;
    BYTE displayBuffer[255];


    if (FirstValidDisk == (ULONG)-1)
    {
       error_portal("No fixed disks could be located.", 10);
       return;
    }
    CurrentDisk = FirstValidDisk;

    disk = CurrentDisk;
    save_menu(menu);
    save_menu(portal);

#if (LINUX_UTIL)
    for (i=3; i < ConsoleScreen.nLines - 2; i++)
       put_char_cleol(&ConsoleScreen, 176 | A_ALTCHARSET, i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
    for (i=3; i < ConsoleScreen.nLines - 2; i++)
       put_char_cleol(&ConsoleScreen, 176, i, CYAN | BGBLUE);
#endif

    if (SystemDisk[disk] && SystemDisk[disk]->PhysicalDiskHandle)
    {
	    
#if (LINUX_UTIL)
       sprintf(pportal_header, "Disk-#%d %s (%02X:%02X) Sectors-0x%X Size-%.0f bytes",
	       (int)disk,
               PhysicalDiskNames[disk],
	       (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle & 0xFF),
	       (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle >> 8),
	       (unsigned int)((SystemDisk[disk]->Cylinders) *
			       SystemDisk[disk]->TracksPerCylinder *
			       SystemDisk[disk]->SectorsPerTrack),
	       (double)SystemDisk[disk]->driveSize);

#else	    
       sprintf(pportal_header, "Disk-#%d Sectors-0x%X Size-%.0f bytes",
	       (int)disk,
	       (unsigned int)((SystemDisk[disk]->Cylinders) *
			       SystemDisk[disk]->TracksPerCylinder *
			       SystemDisk[disk]->SectorsPerTrack),
	       (double)SystemDisk[disk]->driveSize);
#endif
    }
    else
       sprintf(pportal_header, "Disk-#%d not found", (int)disk);

    pportal = make_portal(&ConsoleScreen,
		       pportal_header,
		       0,
		       3,
		       0,
		       13,
		       ConsoleScreen.nCols - 1,
		       10,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       TRUE);
    if (!pportal)
    {
       restore_menu(portal);
       restore_menu(menu);
       return;
    }

    activate_static_portal(pportal);

    pmenu = make_menu(&ConsoleScreen,
		     " Partition Menu Options ",
		     14,
		     22,
		     6,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     menuFunctionPart,
		     0,
		     menuKeyPart,
		     TRUE,
		     0);

    if (!pmenu)
    {
       if (pportal)
       {
	  deactivate_static_portal(pportal);
	  free_portal(pportal);
       }
       restore_menu(portal);
       restore_menu(menu);
       return;
    }

    ScanVolumes();
    disk = CurrentDisk;
    line = 0;
    if (ValidatePartitionExtants(disk))
    {
       log_sprintf(displayBuffer,
	       "disk-(%d) partition table is invalid", (int)disk);
       write_portal(pportal, displayBuffer, line++, 0, YELLOW | BGBLUE);
    }

    if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
    {
       sprintf(displayBuffer,
	       "     Cylinders       : %12u  ", (unsigned int)SystemDisk[disk]->Cylinders);
       write_portal(pportal, displayBuffer, line, 0, YELLOW | BGBLUE);

       sprintf(displayBuffer,
	       "Tracks/Cylinder : %12u", (unsigned int)SystemDisk[disk]->TracksPerCylinder);
       write_portal(pportal, displayBuffer, line++, 39, YELLOW | BGBLUE);

       sprintf(displayBuffer,
	       "     SectorsPerTrack : %12u  ", (unsigned int)SystemDisk[disk]->SectorsPerTrack);
       write_portal(pportal, displayBuffer, line, 0, YELLOW | BGBLUE);

       sprintf(displayBuffer,
	       "BytesPerSector  : %12u", (unsigned int)SystemDisk[disk]->BytesPerSector);
       write_portal(pportal, displayBuffer, line, 39, YELLOW | BGBLUE);

       line += 2;

       if (!ValidatePartitionExtants(disk))
       {
          for (i=0; i < 4; i++)
          {
	     if ((SystemDisk[disk]->PartitionTable[i].SysFlag) &&
	         (SystemDisk[disk]->PartitionTable[i].nSectorsTotal))
	     {
	        if (SystemDisk[disk]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	        {
		   sprintf(displayBuffer,
			"#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB  %s %s",
			    (int)(i + 1),
			    SystemDisk[disk]->PartitionTable[i].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[i]
			     ? "3.x"
			     : "4.x/5.x"));
		   write_portal(pportal, displayBuffer, line++, 2, YELLOW | BGBLUE);
	        }
	        else
	        {
		   sprintf(displayBuffer,
			"#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB  %s",
			    (int)(i + 1),
			    SystemDisk[disk]->PartitionTable[i].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag));
		   write_portal(pportal, displayBuffer, line++, 2, YELLOW | BGBLUE);
	        }
	     }
	     else
	     {
	        sprintf(displayBuffer,
		     "#%d  ------------- Unused ------------- ", (int)(i + 1));
	        write_portal(pportal, displayBuffer, line++, 2, YELLOW | BGBLUE);
	     }
          }
       }
    }
    update_static_portal(pportal);

    add_item_to_menu(pmenu, "Apply Partition Table Changes", 1);
    add_item_to_menu(pmenu, "Select Fixed Disk", 2);
    add_item_to_menu(pmenu, "Create Netware 3.x Partition", 3);
    add_item_to_menu(pmenu, "Create Netware 4.x/5.x Partition", 4);
    add_item_to_menu(pmenu, "Set Active Partition", 5);
    add_item_to_menu(pmenu, "Delete Partition", 6);
    add_item_to_menu(pmenu, "Initialize Partition Table", 7);
    add_item_to_menu(pmenu, "Assign All Free Space to NetWare", 8);

    ccode = activate_menu(pmenu);

    if (pmenu)
       free_menu(pmenu);

    if (pportal)
    {
       deactivate_static_portal(pportal);
       free_portal(pportal);
    }

    restore_menu(portal);
    restore_menu(menu);
    return;

}

BYTE *get_full_ns_string(ULONG ns)
{
    switch (ns)
    {
       case 0:
	  return "MS-DOS";

       case 1:
	  return "Macintosh (MAC)";

       case 2:
	  return "Unix (NFS)";

       case 3:
	  return "FTAM";

       case 4:
	  return "Long Names (LONG)";

       case 5:
	  return "Windows NT";

       case 6:
	  return "";

       default:
	  return "";
    }
}

void build_volume_tables(void)
{
    register ULONG i;

    volume_config_count = 0;
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i] && VolumeTable[i]->VolumePresent)
       {
	  if (volume_config_count < 256)
	  {
	     volume_config_table[volume_config_count] = VolumeTable[i];
	     volume_config_count++;
	  }
       }
    }
    return;
}

ULONG nsFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
    register ULONG i, r, col, vflag;
    register VOLUME *volume;
    BYTE FoundNameSpace[MAX_NAMESPACES];
    BYTE displayBuffer[255];
    ULONG ccode, WorkFlags, message;
    ROOT3X *root3x;
    ROOT *root;
    BYTE *WorkBuffer;
    ULONG saFlag, fcFlag, ndsFlag, dmFlag, adFlag, line;
    BYTE saBuffer[5], fcBuffer[5], ndsBuffer[5];
    BYTE dmBuffer[5], adBuffer[5];

    volume = volume_config_table[index];
    if (!volume)
    {
       error_portal("**** volume table is invalid ****", 10);
       return 0;
    }

    WorkBuffer = NWFSAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!WorkBuffer)
    {
       error_portal("**** could not allocate workbuffer ****", 10);
       return 0;
    }

    NWFSSet(WorkBuffer, 0, IO_BLOCK_SIZE);
    root = (ROOT *) WorkBuffer;
    root3x = (ROOT3X *) root;
    WorkFlags = volume->VolumeFlags;
    saFlag = fcFlag = ndsFlag = dmFlag = adFlag = 0;
    line = 4;

    NWFSSet(saBuffer, 0, 5);
    NWFSSet(fcBuffer, 0, 5);
    NWFSSet(ndsBuffer, 0, 5);
    NWFSSet(dmBuffer, 0, 5);
    NWFSSet(adBuffer, 0, 5);

    if (WorkFlags & NEW_TRUSTEE_COUNT)
    {
       saFlag = (WorkFlags & SUB_ALLOCATION_ON) ? 0 : 1;
       fcFlag = (WorkFlags & FILE_COMPRESSION_ON) ? 0 : 1;
       ndsFlag = (WorkFlags & NDS_FLAG) ? 0 : 1;
    }
    dmFlag = (WorkFlags & DATA_MIGRATION_ON) ? 0 : 1;
    adFlag = (WorkFlags & AUDITING_ON) ? 0 : 1;

    if (WorkFlags & NEW_TRUSTEE_COUNT)
    {
       vflag = make_portal(&ConsoleScreen,
		       0,
		       0,
		       7,
		       8,
		       19,
		       72,
		       11,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

       if (!vflag)
       {
	  NWFSFree(WorkBuffer);
	  return 0;
       }

#if (LINUX_UTIL)
       sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-WriteFlags  F3-Back");
       write_portal_cleol(vflag, displayBuffer, 10, 0, BLUE | BGWHITE);
#else
       sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-WriteFlags  ESC-Back");
       write_portal_cleol(vflag, displayBuffer, 10, 0, BLUE | BGWHITE);
#endif
       activate_static_portal(vflag);

       sprintf(displayBuffer, "Volume Name                 : [%-15s]", volume->VolumeName);
       write_portal(vflag, displayBuffer, 1, 2, BRITEWHITE | BGBLUE);

       sprintf(displayBuffer, "Volume Type                 : %s",
	   ((WorkFlags & NDS_FLAG) || (WorkFlags & NEW_TRUSTEE_COUNT))
	    ? "4.x/5.x" : "3.x");
       write_portal(vflag, displayBuffer, 2, 2, BRITEWHITE | BGBLUE);

       ccode = add_field_to_portal(vflag, line++, 2, WHITE | BGBLUE,
			      "Suballocation               :",
			      strlen("Suballocation               :"),
			      saBuffer,
			      sizeof(saBuffer),
			      OnOff, 2,
			      (WorkFlags & SUB_ALLOCATION_ON) ? 0 : 1,
			      &saFlag,
			      0);
       if (ccode)
       {
	  if (vflag)
	  {
	     deactivate_static_portal(vflag);
	     free_portal(vflag);
	  }
	  NWFSFree(WorkBuffer);
	  return 0;
       }

       ccode = add_field_to_portal(vflag, line++, 2, WHITE | BGBLUE,
			      "File Compression            :",
			      strlen("File Compression            :"),
			      fcBuffer,
			      sizeof(fcBuffer),
			      OnOff, 2,
			      (WorkFlags & FILE_COMPRESSION_ON) ? 0 : 1,
			      &fcFlag,
			      0);
       if (ccode)
       {
	  if (vflag)
	  {
	     deactivate_static_portal(vflag);
	     free_portal(vflag);
	  }
	  NWFSFree(WorkBuffer);
	  return 0;
       }

       ccode = add_field_to_portal(vflag, line++, 2, WHITE | BGBLUE,
			      "Netware Directory Services  :",
			      strlen("Netware Directory Services  :"),
			      ndsBuffer,
			      sizeof(ndsBuffer),
			      OnOff, 2,
			      (WorkFlags & NDS_FLAG) ? 0 : 1,
			      &ndsFlag,
			      0);
       if (ccode)
       {
	  if (vflag)
	  {
	     deactivate_static_portal(vflag);
	     free_portal(vflag);
	  }
	  NWFSFree(WorkBuffer);
	  return 0;
       }
    }
    else
    {
       vflag = make_portal(&ConsoleScreen,
		       0,
		       0,
		       7,
		       8,
		       16,
		       72,
		       8,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

       if (!vflag)
       {
	  NWFSFree(WorkBuffer);
	  return 0;
       }

#if (LINUX_UTIL)
       sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-WriteFlags  F3-Back");
       write_portal_cleol(vflag, displayBuffer, 7, 0, BLUE | BGWHITE);
#else
       sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-WriteFlags  ESC-Back");
       write_portal_cleol(vflag, displayBuffer, 7, 0, BLUE | BGWHITE);
#endif
       activate_static_portal(vflag);

       sprintf(displayBuffer,
	    "Volume Name                 : [%-15s]", volume->VolumeName);
       write_portal(vflag, displayBuffer, 1, 2, BRITEWHITE | BGBLUE);

       sprintf(displayBuffer,
	    "Volume Type                 : %s",
	   ((WorkFlags & NDS_FLAG) || (WorkFlags & NEW_TRUSTEE_COUNT))
	    ? "4.x/5.x" : "3.x");
       write_portal(vflag, displayBuffer, 2, 2, BRITEWHITE | BGBLUE);
    }

    ccode = add_field_to_portal(vflag, line++, 2, WHITE | BGBLUE,
			      "Data Migration              :",
			      strlen("Data Migration              :"),
			      dmBuffer,
			      sizeof(dmBuffer),
			      OnOff, 2,
			      (WorkFlags & DATA_MIGRATION_ON) ? 0 : 1,
			      &dmFlag,
			      0);
    if (ccode)
    {
       if (vflag)
       {
	  deactivate_static_portal(vflag);
	  free_portal(vflag);
       }
       NWFSFree(WorkBuffer);
       return 0;
    }

    ccode = add_field_to_portal(vflag, line++, 2, WHITE | BGBLUE,
			      "Auditing                    :",
			      strlen("Auditing                    :"),
			      adBuffer,
			      sizeof(adBuffer),
			      OnOff, 2,
			      (WorkFlags & AUDITING_ON) ? 0 : 1,
			      &adFlag,
			      0);
    if (ccode)
    {
       if (vflag)
       {
	    deactivate_static_portal(vflag);
	    free_portal(vflag);
       }
       NWFSFree(WorkBuffer);
       return 0;
    }

    ccode = input_portal_fields(vflag);
    if (ccode)
    {
       if (vflag)
       {
	    deactivate_static_portal(vflag);
	    free_portal(vflag);
       }
       NWFSFree(WorkBuffer);
       return 0;
    }

    if (vflag)
    {
       deactivate_static_portal(vflag);
       free_portal(vflag);
    }

    if (WorkFlags & NEW_TRUSTEE_COUNT)
    {
       if (!saFlag)
	  WorkFlags |= SUB_ALLOCATION_ON;
       else
	  if (WorkFlags & SUB_ALLOCATION_ON)
	     error_portal("**** Once Enabled, Suballocation Cannot Be Disabled ****", 10);

       if (!fcFlag)
	  WorkFlags |= FILE_COMPRESSION_ON;
       else
	  WorkFlags &= ~FILE_COMPRESSION_ON;

       WorkFlags |= NDS_FLAG; // do not allow this flag to be disabled
    }
    if (!dmFlag)
       WorkFlags |= DATA_MIGRATION_ON;
    else
       WorkFlags &= ~DATA_MIGRATION_ON;

    if (!adFlag)
       WorkFlags |= AUDITING_ON;
    else
       WorkFlags &= ~AUDITING_ON;

    nwvp_vpartition_open(volume->nwvp_handle);

    ccode = nwvp_vpartition_block_read(volume->nwvp_handle,
			(volume->FirstDirectory * volume->BlocksPerCluster),
			1, WorkBuffer);
    if (ccode)
    {
       nwvp_vpartition_close(volume->nwvp_handle);
       error_portal("**** error reading volume root ****", 10);
       NWFSFree(WorkBuffer);
       return 0;
    }

    volume->VolumeFlags = WorkFlags;
    if (volume->VolumeFlags & NEW_TRUSTEE_COUNT)
       root->VolumeFlags = (BYTE)volume->VolumeFlags;
    else
       root3x->VolumeFlags = (BYTE)volume->VolumeFlags;

    log_sprintf(displayBuffer, "Writing Volume %s ...", volume->VolumeName);
    message = create_message_portal(displayBuffer, 10, YELLOW | BGBLUE);

    ccode = nwvp_vpartition_block_write(volume->nwvp_handle,
			(volume->FirstDirectory * volume->BlocksPerCluster),
			1, WorkBuffer);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       nwvp_vpartition_close(volume->nwvp_handle);
       error_portal("**** error writing volume root (primary) ****", 10);
       NWFSFree(WorkBuffer);
       return 0;
    }

    ccode = nwvp_vpartition_block_write(volume->nwvp_handle,
			(volume->SecondDirectory * volume->BlocksPerCluster),
			1, WorkBuffer);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       nwvp_vpartition_close(volume->nwvp_handle);
       error_portal("**** error writing volume root (mirror) ****", 10);
       NWFSFree(WorkBuffer);
       return 0;
    }

    if (message)
       close_message_portal(message);

    nwvp_vpartition_close(volume->nwvp_handle);

    NWFSFree(WorkBuffer);

    NWFSSet(FoundNameSpace, 0, MAX_NAMESPACES);
    build_volume_tables();
    for (i=0; i < volume_config_count; i++)
    {
       col = 0;
       volume = volume_config_table[i];
       if (volume)
       {
	  sprintf(displayBuffer, "[%-15s]    %s     %s      %s    %s   ",
	       volume->VolumeName,
	       (volume->VolumeFlags & FILE_COMPRESSION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & SUB_ALLOCATION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & DATA_MIGRATION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & AUDITING_ON)
		? "YES" : "NO ");

	  write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
	  col += strlen(displayBuffer);

	  if (volume->NameSpaceCount)
	  {
	     NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	     write_portal_cleol(nportal, " ", i, col, YELLOW | BGBLUE);

	     for (r=0; r < (volume->NameSpaceCount & 0xF); r++)
	     {
		// don't display duplicate namespaces, report error.
		if (r && (FoundNameSpace[volume->NameSpaceID[r] & 0xF]))
		{
		   if ((r + 1) >= (volume->NameSpaceCount & 0xF))
		      sprintf(displayBuffer, "???");
		   else
		      sprintf(displayBuffer, "???,");

		   write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
		   col += strlen(displayBuffer);

		   FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
		   continue;
		}

		FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
		if ((r + 1) >= (volume->NameSpaceCount & 0xF))
		   sprintf(displayBuffer, "%s", NSDescription(volume->NameSpaceID[r]));
		else
		   sprintf(displayBuffer, "%s,", NSDescription(volume->NameSpaceID[r]));

		write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
		col += strlen(displayBuffer);
	     }
	  }
	  else
	  {
	     sprintf(displayBuffer, "DOS");

	     write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
	     col += strlen(displayBuffer);
	  }
       }
    }
    SyncDisks();
    return 0;
}


ULONG NWConfigAddSpecificVolumeNamespace(VOLUME *volume, ULONG namespace)
{
    register ULONG Dir1Size, Dir2Size, i, j, k, ccode, owner;
    register ULONG DirCount, DirBlocks, cbytes, DirsPerBlock, DirTotal;
    register ULONG PrevLink, NameLink, NextLink, LinkCount, message;
    LONGLONG VolumeSpace, NameSpaceSize;
    register DOS *dos, *new, *prev, *name, *base;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register TRUSTEE *trustee;
    register USER *user;
    register ROOT *root;
    register ROOT3X *root3x;
    register ULONG MacID, LongID, NfsID, DosID, NtID, NewDirNo;
    ULONG retRCode;
    BYTE FoundNameSpace[MAX_NAMESPACES];
    BYTE displayBuffer[255];

    if (namespace == DOS_NAME_SPACE)
       return -1;

    ccode = MountRawVolume(volume);
    if (ccode)
       return NwVolumeCorrupt;

    dos = NWFSCacheAlloc(IO_BLOCK_SIZE, DIR_WORKSPACE_TAG);
    if (!dos)
    {
       DismountRawVolume(volume);
       return NwInsufficientResources;
    }

    new = NWFSCacheAlloc(IO_BLOCK_SIZE, DIR_WORKSPACE_TAG);
    if (!new)
    {
       NWFSFree(dos);
       DismountRawVolume(volume);
       return NwInsufficientResources;
    }
    prev     = (DOS *)&new[1];  // create temporary directory workspaces
    name     = (DOS *)&new[2];
    base     = (DOS *)&new[3];
    mac      = (MACINTOSH *)&new[4];
    nfs      = (NFS *)&new[5];
    longname = (LONGNAME *)&new[6];
    nt       = (NTNAME *)&new[7];

    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);

    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return NwDirectoryCorrupt;
    }

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    DirBlocks = (Dir1Size + (volume->BlockSize - 1)) / volume->BlockSize;
    DirTotal = (Dir1Size + (sizeof(DOS) - 1)) / sizeof(DOS);

    log_sprintf(displayBuffer,
	    "Analyzing space requirements for the %s namespace ",
	     NSDescription(namespace));
    message = create_message_portal(displayBuffer, 10, YELLOW | BGBLUE);

    ccode = UtilInitializeDirAssignHash(volume);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       error_portal("**** nwfs:  error during assign hash init ****", 10);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    for (DirCount = i = 0; i < DirBlocks; i++)
    {
       cbytes = NWReadFile(volume,
			      &volume->FirstDirectory,
			      0,
			      Dir1Size,
			      i * volume->BlockSize,
			      (BYTE *)dos,
			      volume->BlockSize,
			      0,
			      0,
			      &retRCode,
			      KERNEL_ADDRESS_SPACE,
			      0,
			      0,
			      TRUE);
       if (cbytes != volume->BlockSize)
       {
	  if (message)
	     close_message_portal(message);
	  error_portal("**** nwfs:  error reading directory file ****", 10);
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  NWFSFree(new);
	  return -1;
       }

       for (owner = (ULONG) -1, j=0; j < DirsPerBlock; j++)
       {
	  switch (dos[j].Subdirectory)
	  {
	     case ROOT_NODE:
		if (owner == (ULONG) -1)
		   owner = 0;
		root = (ROOT *) &dos[j];
		if (root->NameSpace == DOS_NAME_SPACE)
		   DirCount++;
		break;

	     case SUBALLOC_NODE:
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     case FREE_NODE:
		break;

	     case RESTRICTION_NODE:
		user = (USER *) &dos[j];
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     case TRUSTEE_NODE:
		trustee = (TRUSTEE *) &dos[j];
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     default:
		if (owner == (ULONG) -1)
		   owner = dos[j].Subdirectory;

		if (dos[j].NameSpace == DOS_NAME_SPACE)
		   DirCount++;
		break;
	  }
       }

       ccode = UtilCreateDirAssignEntry(volume, owner, i, dos);
       if (ccode)
       {
	  if (message)
	     close_message_portal(message);
	  error_portal("**** nwfs:  error creating dir assign hash ****", 10);
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  NWFSFree(new);
	  return -1;
       }
    }

    if (message)
       close_message_portal(message);

    VolumeSpace = (LONGLONG)((LONGLONG)volume->VolumeAllocatedClusters *
			     (LONGLONG)volume->ClusterSize);
    NameSpaceSize = (LONGLONG)((LONGLONG)DirCount * (LONGLONG)sizeof(ROOT));

    if (NameSpaceSize >= (VolumeSpace + sizeof(ROOT)))
    {
       error_portal("**** Insufficient space available to add namespace ****", 10);
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    sprintf(displayBuffer,
	    " %s Namespace Needs %u K of Disk Space.  Continue ?",
	    NSDescription(namespace),
	    (unsigned int)((LONGLONG)NameSpaceSize /
			   (LONGLONG) 1024));
    ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
    if (!ccode)
    {
       error_portal("*** Add Namespace Operation Cancelled ***", 10);
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return 0;
    }

    log_sprintf(displayBuffer,
	    "Adding %s namespace to volume %s ...",
	    NSDescription(namespace), volume->VolumeName);
    message = create_message_portal(displayBuffer, 10, YELLOW | BGBLUE);

    // read the root directory record
    root = (ROOT *) dos;
    root3x = (ROOT3X *) dos;
    ccode = ReadDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       error_portal("**** nwfs:  error reading volume root ****", 10);
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    // see if this is a Netware 4.x/5.x volume
    if ((volume->VolumeFlags & NDS_FLAG) ||
	(volume->VolumeFlags & NEW_TRUSTEE_COUNT))
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  duplicate namespace error ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (!j && volume->NameSpaceID[j])
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  dos namespace not present ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  multiple namespace error ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // add the namespace to the list in the volume table
       volume->NameSpaceID[volume->NameSpaceCount] = namespace;
       volume->NameSpaceCount++;
       root->NameSpaceCount = (BYTE) volume->NameSpaceCount;

       NWFSSet(&root->SupportedNameSpacesNibble[0], 0, 6);
       for (j=0, k=0; j < volume->NameSpaceCount; j++)
       {
	  if (j & 1)
	  {
	     root->SupportedNameSpacesNibble[k] |=
		((volume->NameSpaceID[j] << 4) & 0xF0);
	     k++;
	  }
	  else
	  {
	     root->SupportedNameSpacesNibble[k] |=
		(volume->NameSpaceID[j] & 0xF);
	  }
       }
    }
    else
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  duplicate namespace error ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (!j && volume->NameSpaceID[j])
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  dos namespace not present ****", 0);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  multiple namespace error ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // add the namespace to the list in the volume table
       volume->NameSpaceID[volume->NameSpaceCount] = namespace;
       volume->NameSpaceCount++;
       root3x->NameSpaceCount = (BYTE) volume->NameSpaceCount;

       // clear the current namespace entries in the root
       // entry and reset.

       NWFSSet(&root3x->NameSpaceTable[0], 0, 10);
       for (j=0; j < volume->NameSpaceCount; j++)
	  root3x->NameSpaceTable[j] = (BYTE)volume->NameSpaceID[j];
    }

    // write the modified root entry
    ccode = WriteDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       error_portal("**** nwfs:  error writing volume root ****", 10);
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    // now add the new namespace records.  DirTotal should be the upward
    // limit of detected DOS namespace records.  While we are allocating
    // new records to add the namespace, the directory file will expand
    // beyond this point, but this should be ok.

    for (i=0; i < DirTotal; i++)
    {
       ccode = ReadDirectoryRecord(volume, dos, i);
       if (ccode)
       {
	  if (message)
	     close_message_portal(message);
	  error_portal("**** nwfs:  error reading volume directory ****", 10);
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  NWFSFree(new);
	  return -1;
       }

       NtID = MacID = DosID = LongID = NfsID = (ULONG) -1;
       if ((dos->Subdirectory != FREE_NODE)        &&
	   (dos->Subdirectory != SUBALLOC_NODE)    &&
	   (dos->Subdirectory != RESTRICTION_NODE) &&
	   (dos->Subdirectory != TRUSTEE_NODE)     &&
	   (dos->NameSpace == DOS_NAME_SPACE))
       {
	  // copy the DOS root entry and store in temporary workspace
	  NWFSCopy(base, dos, sizeof(DOS));

	  NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	  PrevLink = i;
	  LinkCount = 1;
	  NameLink = dos->NameList;
	  DosID = i;

	  // run the entire name list chain and look for duplicate
	  // namespace records.  Also check for invalid name list
	  // linkage.

	  while (NameLink)
	  {
	     if (LinkCount++ > volume->NameSpaceCount)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  invalid name list detected (add)****", 10);
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }

	     if (NameLink > DirTotal)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  directory number out of range (add namespace) ****", 10);
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }

	     if (FoundNameSpace[dos->NameSpace & 0xF])
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  multiple name space error ****", 10);
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }
	     FoundNameSpace[dos->NameSpace & 0xF] = TRUE;

	     ccode = ReadDirectoryRecord(volume, dos, NameLink);
	     if (ccode)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  error reading volume directory ****", 10);
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }

	     // record directory numbers for each namespace we encounter
	     // and make a copy of each of the diretory records
	     switch (dos->NameSpace)
	     {
		case MAC_NAME_SPACE:
		   if (MacID == (ULONG) -1)
		      MacID = NameLink;
		   NWFSCopy(mac, dos, sizeof(DOS));
		   break;

		case LONG_NAME_SPACE:
		   if (LongID == (ULONG) -1)
		      LongID = NameLink;
		   NWFSCopy(longname, dos, sizeof(DOS));
		   break;

		case NT_NAME_SPACE:
		   if (NtID == (ULONG) -1)
		      NtID = NameLink;
		   NWFSCopy(nt, dos, sizeof(DOS));
		   break;

		case UNIX_NAME_SPACE:
		   if (NfsID == (ULONG) -1)
		      NfsID = NameLink;
		   NWFSCopy(nfs, dos, sizeof(DOS));
		   break;

		default:
		   break;
	     }

	     // save the link to the next namespace record
	     NextLink = dos->NameList;
	     PrevLink = NameLink;
	     NameLink = NextLink;
	  }

	  // select a name record to use to create the name for the new
	  // record.  If we have a LONG or NFS namespace, use this
	  // namespace to provide the name for the new record, otherwise
	  // default to NT and MAC namespaces.  If none of the cases
	  // are detected, then by default, we use the DOS namespace.

	  if ((namespace != LONG_NAME_SPACE) && (LongID != (ULONG) -1))
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   longname->FileName,
					   longname->FileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }
	  else
	  if ((namespace != UNIX_NAME_SPACE) && (NfsID != (ULONG) -1))
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   nfs->FileName,
					   nfs->TotalFileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }
	  else
	  if ((namespace != NT_NAME_SPACE) && (NtID != (ULONG) -1))
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   nt->FileName,
					   nt->FileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }
	  else
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   base->FileName,
					   base->FileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }

	  // save a copy of the last namespace record in this chain.
	  NWFSCopy(prev, dos, sizeof(DOS));

	  if (ccode)
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  error formatting namespace record ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }

	  // now try to allocate a new directory record for this namespace
	  // entry.

	  if (base->Subdirectory == ROOT_NODE)
	     NewDirNo = UtilAllocateDirectoryRecord(volume, 0);
	  else
	     NewDirNo = UtilAllocateDirectoryRecord(volume, base->Subdirectory);

	  if (NewDirNo == (ULONG) -1)
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  error allocating directory record ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }

	  // set the last record in the namespace chain to point to this
	  // record and set the new record to point to the primary entry.

	  prev->NameList = NewDirNo;
	  new->NameList = 0;
	  new->PrimaryEntry = prev->PrimaryEntry;

	  ccode = WriteDirectoryRecord(volume, prev, PrevLink);
	  if (ccode)
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  error writing volume directory ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }

	  ccode = WriteDirectoryRecord(volume, new, NewDirNo);
	  if (ccode)
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  error writing volume directory ****", 10);
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
       }
    }
    UtilFreeDirAssignHash(volume);
    DismountRawVolume(volume);
    NWFSFree(dos);
    NWFSFree(new);

    FlushVolumeLRU(volume);

    SyncDisks();

    if (message)
       close_message_portal(message);

    return 0;
}

ULONG NWConfigRemoveSpecificVolumeNamespace(VOLUME *volume, ULONG namespace)
{
    register ULONG Dir1Size, Dir2Size, i, j, k, ccode, pFlag, message;
    register ULONG DirTotal, PrevLink, NameLink, NextLink, LinkCount;
    register DOS *dos;
    register ROOT *root;
    register ROOT3X *root3x;
    BYTE displayBuffer[255];
    BYTE FoundNameSpace[MAX_NAMESPACES];

    if (namespace == DOS_NAME_SPACE)
       return -1;

    ccode = MountRawVolume(volume);
    if (ccode)
       return NwVolumeCorrupt;

    dos = NWFSCacheAlloc(sizeof(ROOT), DIR_WORKSPACE_TAG);
    if (!dos)
    {
       DismountRawVolume(volume);
       return NwInsufficientResources;
    }

    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);

    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       DismountRawVolume(volume);
       NWFSFree(dos);
       return NwDirectoryCorrupt;
    }

    DirTotal = Dir1Size / sizeof(ROOT);

    log_sprintf(displayBuffer,
	    "Removing %s namespace from volume %s ...",
	     NSDescription(namespace), volume->VolumeName);
    message = create_message_portal(displayBuffer, 10, YELLOW | BGBLUE);

    // read the root directory record
    root = (ROOT *) dos;
    root3x = (ROOT3X *) dos;
    ccode = ReadDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       error_portal("**** nwfs:  error reading volume root ****", 10);
       DismountRawVolume(volume);
       NWFSFree(dos);
       return -1;
    }

    // see if this is a Netware 4.x/5.x volume
    if ((volume->VolumeFlags & NDS_FLAG) ||
	(volume->VolumeFlags & NEW_TRUSTEE_COUNT))
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (!j && volume->NameSpaceID[j])
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  dos namespace not present ****", 10);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  multiple namespace error ****", 10);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // remove the namespace from the list in the volume table
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     for (; j < volume->NameSpaceCount; j++)
	     {
		if ((j + 1) >= volume->NameSpaceCount)
		   volume->NameSpaceID[j] = 0;
		else
		   volume->NameSpaceID[j] = volume->NameSpaceID[j + 1];
	     }
	  }
       }

       // don't go negative if our count is zero
       if (volume->NameSpaceCount)
	  volume->NameSpaceCount--;

       // if only the DOS namespace is present, then we set the
       // namespace count to zero (Netware specific behavior).
       if (volume->NameSpaceCount == 1)
	  root->NameSpaceCount = 0;
       else
	  root->NameSpaceCount = (BYTE) volume->NameSpaceCount;


       // clear the current namespace entries in the root
       // entry and reset.

       NWFSSet(&root->SupportedNameSpacesNibble[0], 0, 6);
       for (j=0, k=0; j < volume->NameSpaceCount; j++)
       {
	  if (j & 1)
	  {
	     root->SupportedNameSpacesNibble[k] |=
		((volume->NameSpaceID[j] << 4) & 0xF0);
	     k++;
	  }
	  else
	  {
	     root->SupportedNameSpacesNibble[k] |=
		(volume->NameSpaceID[j] & 0xF);
	  }
       }
    }
    else
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (!j && volume->NameSpaceID[j])
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  dos namespace not present ****", 10);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     if (message)
		close_message_portal(message);
	     error_portal("**** nwfs:  multiple namespace error ****", 10);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // remove the namespace from the list in the volume table
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     for (; j < volume->NameSpaceCount; j++)
	     {
		if ((j + 1) >= volume->NameSpaceCount)
		   volume->NameSpaceID[j] = 0;
		else
		   volume->NameSpaceID[j] = volume->NameSpaceID[j + 1];
	     }
	  }
       }

       // don't go negative if our count is zero
       if (volume->NameSpaceCount)
	  volume->NameSpaceCount--;

       // if only the DOS namespace is present, then we set the
       // namespace count to zero (Netware specific behavior).
       if (volume->NameSpaceCount == 1)
	  root3x->NameSpaceCount = 0;
       else
	  root3x->NameSpaceCount = (BYTE) volume->NameSpaceCount;

       // clear the current namespace entries in the root
       // entry and reset.

       NWFSSet(&root3x->NameSpaceTable[0], 0, 10);
       for (j=0; j < volume->NameSpaceCount; j++)
	  root3x->NameSpaceTable[j] = (BYTE)volume->NameSpaceID[j];
    }

    // write the modified root entry
    ccode = WriteDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       if (message)
	  close_message_portal(message);
       error_portal("**** nwfs:  error writing volume root ****", 10);
       DismountRawVolume(volume);
       NWFSFree(dos);
       return -1;
    }

    // now remove the affected namespace records
    for (i=0; i < DirTotal; i++)
    {
       ccode = ReadDirectoryRecord(volume, dos, i);
       if (ccode)
       {
	  if (message)
	     close_message_portal(message);
	  error_portal("**** nwfs:  error reading volume directory ****", 10);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  return -1;
       }

       if ((dos->Subdirectory != FREE_NODE)        &&
	   (dos->Subdirectory != SUBALLOC_NODE)    &&
	   (dos->Subdirectory != RESTRICTION_NODE) &&
	   (dos->Subdirectory != TRUSTEE_NODE)     &&
	   (dos->NameSpace == DOS_NAME_SPACE))
       {
	  NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	  pFlag = 0;
	  PrevLink = i;
	  LinkCount = 1;
	  NameLink = dos->NameList;

	  while (NameLink)
	  {
	     if (LinkCount++ > volume->NameSpaceCount)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  invalid name list detected (add) ****", 10);
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     if (NameLink > DirTotal)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  directory number out of range (remove) ****", 10);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     if (FoundNameSpace[dos->NameSpace & 0xF])
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  multiple name space error ****", 10);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }
	     FoundNameSpace[dos->NameSpace & 0xF] = TRUE;

	     ccode = ReadDirectoryRecord(volume, dos, NameLink);
	     if (ccode)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  error reading volume directory ****", 10);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     // save the link to the next namespace record
	     NextLink = dos->NameList;

	     if (dos->NameSpace == namespace)
	     {
		// if this record is designated as primary namespace,
		// then we must set the DOS_NAME_SPACE record as the
		// new primary.
		if (dos->Flags & PRIMARY_NAMESPACE)
		   pFlag = TRUE;

		// free this record
		NWFSSet(dos, 0, sizeof(DOS));
		dos->Subdirectory = (ULONG) FREE_NODE;

		// mark this record as free
		ccode = WriteDirectoryRecord(volume, dos, NameLink);
		if (ccode)
		{
		   if (message)
		      close_message_portal(message);
		   error_portal("**** nwfs:  error writing volume directory ****", 10);
		   DismountRawVolume(volume);
		   NWFSFree(dos);
		   return -1;
		}

		// read the previous record
		ccode = ReadDirectoryRecord(volume, dos, PrevLink);
		if (ccode)
		{
		   if (message)
		      close_message_portal(message);
		   error_portal("**** nwfs:  error reading volume directory ****", 10);
		   DismountRawVolume(volume);
		   NWFSFree(dos);
		   return -1;
		}

		// update previous namespace forward link to the next link
		// for this namespace record
		dos->NameList = NextLink;

		// write the previous record
		ccode = WriteDirectoryRecord(volume, dos, PrevLink);
		if (ccode)
		{
		   if (message)
		      close_message_portal(message);
		   error_portal("**** nwfs:  error writing volume directory ****", 10);
		   DismountRawVolume(volume);
		   NWFSFree(dos);
		   return -1;
		}
		break;
	     }
	     PrevLink = NameLink;
	     NameLink = NextLink;
	  }

	  // if we are removing a namespace record that was designated
	  // as PRIMARY_NAMESPACE, then set the DOS_NAME_SPACE entry
	  // as the new primary namespace record.

	  if (pFlag)
	  {
	     ccode = ReadDirectoryRecord(volume, dos, i);
	     if (ccode)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  error reading volume directory ****", 10);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     dos->Flags |= PRIMARY_NAMESPACE;
	     ccode = WriteDirectoryRecord(volume, dos, i);
	     if (ccode)
	     {
		if (message)
		   close_message_portal(message);
		error_portal("**** nwfs:  error writing volume directory ****", 10);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }
	  }
       }
    }
    DismountRawVolume(volume);
    NWFSFree(dos);

    FlushVolumeLRU(volume);

    SyncDisks();

    if (message)
       close_message_portal(message);

    return 0;
}

ULONG nsHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    ULONG help, nsmenu;
    BYTE displayBuffer[255];
    register ULONG i, r, col;
    register VOLUME *volume;
    ULONG AvailNS, ccode;
    ULONG CountedNamespaces;
    BYTE NSID;
    BYTE NSFlag[MAX_NAMESPACES];
    ULONG NSTotal, value;
    BYTE NSArray[MAX_NAMESPACES];
    BYTE FoundNameSpace[MAX_NAMESPACES];

    NWFSSet(FoundNameSpace, 0, MAX_NAMESPACES);

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  help = make_portal(&ConsoleScreen,
		       " Help for Volume Namespace Management ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       256,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(NamespaceHelp) / sizeof (BYTE *); i++)
	     write_portal_cleol(help, NamespaceHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Menu  INS-Add Namespace  DEL-Delete Namespace  ENTER-Change Flags");
#else
	  sprintf(displayBuffer, "  ESC-Menu  INS-Add Namespace  DEL-Delete Namespace  ENTER-Change Flags");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       case INS:
	  volume = volume_config_table[index];
	  if (!volume)
	  {
	     error_portal("**** volume table is invalid ****", 10);
	     break;
	  }

	  NWFSSet(&NSFlag[0], 0, MAX_NAMESPACES);
	  NWFSSet(&NSArray[0], 0, MAX_NAMESPACES);

	  // assume 6 namespaces are allowed
	  for (AvailNS = 6, i=0; i < (volume->NameSpaceCount & 0x0F); i++)
	  {
	     NSID = (BYTE)(volume->NameSpaceID[i] & 0x0F);
	     if (NSFlag[NSID])
	     {
		error_portal("**** Multiple Namespace entry error ****", 10);
		break;
	     }
	     NSFlag[NSID] = TRUE;
	     if (AvailNS)
		AvailNS--;
	  }

	  for (NSTotal = i = 0; i < 6; i++)
	  {
	     if (!NSFlag[i])
	     {
		if ((i != FTAM_NAME_SPACE) && (i != NT_NAME_SPACE))
		{
		   NSArray[NSTotal] = (BYTE)i;
		   NSTotal++;
		}
	     }
	  }

	  nsmenu = make_menu(&ConsoleScreen,
			  "  Select a Namespace to Add ",
			  8,
			  24,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  0);

	  if (!nsmenu)
	     break;

	  for (i=0; i < NSTotal; i++)
	     add_item_to_menu(nsmenu, get_full_ns_string(NSArray[i]), i);

	  value = activate_menu(nsmenu);

	  free_menu(nsmenu);

	  if (value == (ULONG) -1)
	     break;

	  if (value >= NSTotal)
	  {
	     error_portal("**** invalid namespace option ****", 10);
	     break;
	  }

	  sprintf(displayBuffer, "  Add Namespace %s ?",
		get_full_ns_string(NSArray[value]));

	  ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	  if (!ccode)
	     break;

	  ccode = NWConfigAddSpecificVolumeNamespace(volume, NSArray[value]);
	  if (ccode)
	  {
	     log_sprintf(displayBuffer,
			"**** nwfs:  error (%d) adding namespace %s",
			(int) ccode, get_full_ns_string(NSArray[value]));
	     error_portal(displayBuffer, 10);
	  }
	  break;

       case DEL:
	  volume = volume_config_table[index];
	  if (!volume)
	  {
	     error_portal("**** volume table is invalid ****", 10);
	     break;
	  }

	  if (volume->NameSpaceCount <= 1)
	  {
	     error_portal("**** no valid namespace to remove ****", 10);
	     break;
	  }

	  nsmenu = make_menu(&ConsoleScreen,
			  "  Select a Namespace to Remove ",
			  8,
			  23,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  0);

	  if (!nsmenu)
	     break;

	  // start with the second valid namespace entry
	  for (CountedNamespaces = 0, i=0; i < volume->NameSpaceCount; i++)
	  {
	     if (volume->NameSpaceID[i])
	     {
		add_item_to_menu(nsmenu,
		       get_full_ns_string(volume->NameSpaceID[i]), i);
		CountedNamespaces++;
	     }
	  }

	  value = activate_menu(nsmenu);

	  free_menu(nsmenu);

	  if (value == (ULONG) -1)
	     break;

	  if (value > CountedNamespaces)
	  {
	     error_portal("**** invalid namespace option ****", 10);
	     break;
	  }

	  if (volume->NameSpaceID[value] == DOS_NAME_SPACE)
	  {
	     error_portal("**** Removing DOS Namespace not allowed ****", 10);
	     break;
	  }

	  NSID = (BYTE)volume->NameSpaceID[value];
	  sprintf(displayBuffer, "  Remove Namespace %s ?",
		  get_full_ns_string(NSID));

	  ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	  if (!ccode)
	     break;

	  ccode = NWConfigRemoveSpecificVolumeNamespace(volume, NSID);
	  if (ccode)
	  {
	     log_sprintf(displayBuffer, "nwfs:  error (%d) removing namespace %s",
		     (int) ccode, get_full_ns_string(NSID));
	     error_portal(displayBuffer, 10);
	  }
	  break;

       default:
	  break;
    }

    build_volume_tables();
    for (i=0; i < volume_config_count; i++)
    {
       col = 0;
       volume = volume_config_table[i];
       if (volume)
       {
	  sprintf(displayBuffer, "[%-15s]    %s     %s      %s    %s   ",
	       volume->VolumeName,
	       (volume->VolumeFlags & FILE_COMPRESSION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & SUB_ALLOCATION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & DATA_MIGRATION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & AUDITING_ON)
		? "YES" : "NO ");

	  write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
	  col += strlen(displayBuffer);

	  if (volume->NameSpaceCount)
	  {
	     NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	     write_portal_cleol(nportal, " ", i, col, YELLOW | BGBLUE);
	     for (r=0; r < (volume->NameSpaceCount & 0xF); r++)
	     {
		// don't display duplicate namespaces, report error.
		if (r && (FoundNameSpace[volume->NameSpaceID[r] & 0xF]))
		{
		   if ((r + 1) >= (volume->NameSpaceCount & 0xF))
		      sprintf(displayBuffer, "???");
		   else
		      sprintf(displayBuffer, "???,");

		   write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
		   col += strlen(displayBuffer);

		   FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
		   continue;
		}

		FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
		if ((r + 1) >= (volume->NameSpaceCount & 0xF))
		   sprintf(displayBuffer, "%s", NSDescription(volume->NameSpaceID[r]));
		else
		   sprintf(displayBuffer, "%s,", NSDescription(volume->NameSpaceID[r]));

		write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
		col += strlen(displayBuffer);
	     }
	  }
	  else
	  {
	     sprintf(displayBuffer, "DOS");

	     write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
	     col += strlen(displayBuffer);
	  }
       }
    }
    return 0;
}

void NWConfigEditNamespaces(void)
{
    register ULONG i, r, col;
    register VOLUME *volume;
    BYTE FoundNameSpace[MAX_NAMESPACES];
    BYTE displayBuffer[255];

    NWFSSet(FoundNameSpace, 0, MAX_NAMESPACES);

    nportal = make_portal(&ConsoleScreen,
		       "Volume            COMPRESS SUBALLOC MIGRATE AUDIT  NameSpaces             ",
		       0,
		       3,
		       0,
		       ConsoleScreen.nLines - 2,
		       ConsoleScreen.nCols - 1,
		       MAX_PARTITIONS,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       nsFunction,
		       0,
		       nsHandler,
		       TRUE);
    if (!nportal)
       return;

    build_volume_tables();
    for (i=0; i < volume_config_count; i++)
    {
       col = 0;
       volume = volume_config_table[i];
       if (volume)
       {
	  write_portal_cleol(nportal, " ", i, col, YELLOW | BGBLUE);
	  sprintf(displayBuffer, "[%-15s]    %s     %s      %s    %s   ",
	       volume->VolumeName,
	       (volume->VolumeFlags & FILE_COMPRESSION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & SUB_ALLOCATION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & DATA_MIGRATION_ON)
		? "YES" : "NO ",
	       (volume->VolumeFlags & AUDITING_ON)
		? "YES" : "NO ");

	  write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
	  col += strlen(displayBuffer);

	  if (volume->NameSpaceCount)
	  {
	     NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	     write_portal_cleol(nportal, " ", i, col, YELLOW | BGBLUE);

	     for (r=0; r < (volume->NameSpaceCount & 0xF); r++)
	     {
		// don't display duplicate namespaces, report error.
		if (r && (FoundNameSpace[volume->NameSpaceID[r] & 0xF]))
		{
		   if ((r + 1) >= (volume->NameSpaceCount & 0xF))
		      sprintf(displayBuffer, "???");
		   else
		      sprintf(displayBuffer, "???,");

		   write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
		   col += strlen(displayBuffer);

		   FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
		   continue;
		}

		FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
		if ((r + 1) >= (volume->NameSpaceCount & 0xF))
		   sprintf(displayBuffer, "%s", NSDescription(volume->NameSpaceID[r]));
		else
		   sprintf(displayBuffer, "%s,", NSDescription(volume->NameSpaceID[r]));

		write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
		col += strlen(displayBuffer);
	     }
	  }
	  else
	  {
	     sprintf(displayBuffer, "DOS");

	     write_portal(nportal, displayBuffer, i, col, YELLOW | BGBLUE);
	     col += strlen(displayBuffer);
	  }
       }
    }

    activate_portal(nportal);

    if (nportal)
    {
       deactivate_static_portal(nportal);
       free_portal(nportal);
    }

}

ULONG mirrorFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
    BYTE workBuffer[12];
    BYTE displayBuffer[255];
    ULONG block_count, iportal, ccode, i;
    ULONG raw_ids[4];

    if (index < hotfix_table_count)
    {
       if ((hotfix_table[index].segment_count== 0) && (hotfix_table[index].mirror_count == 1))
       {
	  register ULONG DataSize;

	  if (hotfix_table[index].physical_block_count < 101)
	  {
	     error_portal("****  Invalid Size ****", 10);
	     return 0;
	  }

	  NWFSSet(workBuffer, 0, sizeof(workBuffer));

	  iportal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       10,
		       5,
		       14,
		       75,
		       3,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	  if (!iportal)
	     return 0;

	  activate_static_portal(iportal);

          NWFSSet(workBuffer, 0, sizeof(workBuffer));

	  sprintf(displayBuffer,
		  "Enter Logical Data Area Size (up to %uMB): ",
		  (unsigned int)
		    (((LONGLONG)(hotfix_table[index].physical_block_count - 100) *
		      (LONGLONG)IO_BLOCK_SIZE) /
		      (LONGLONG)0x100000));

	  ccode = add_field_to_portal(iportal, 1, 2, WHITE | BGBLUE,
			 displayBuffer, strlen(displayBuffer),
			 workBuffer, sizeof(workBuffer),
			 0, 0, 0, 0, FIELD_ENTRY);
	  if (ccode)
	  {
	     error_portal("****  error:  could not allocate memory ****", 10);
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     return 0;
	  }

	  ccode = input_portal_fields(iportal);
	  if (ccode)
	  {
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     return 0;
	  }

	  if (iportal)
	  {
	     deactivate_static_portal(iportal);
	     free_portal(iportal);
	  }

	  DataSize = atol(workBuffer);
	  block_count = (ULONG)(((LONGLONG)DataSize * (LONGLONG)0x100000) /
				 (LONGLONG)IO_BLOCK_SIZE);
	  if (block_count)
	  {
	     if ((block_count + 100) < hotfix_table[index].physical_block_count)
	     {
		ULONG message;

		raw_ids[0] = hotfix_table[index].rpart_handle;
		raw_ids[1] = 0xFFFFFFFF;
		raw_ids[2] = 0xFFFFFFFF;
		raw_ids[3] = 0xFFFFFFFF;

		message = create_message_portal("Reformatting Partition ...",
						10,
						YELLOW | BGBLUE);

		nwvp_lpartition_unformat(hotfix_table[index].lpart_handle);
		nwvp_lpartition_format(&hotfix_table[index].lpart_handle,
						   block_count, &raw_ids[0]);


		if (message)
		   close_message_portal(message);

		SyncDisks();
		nwconfig_build_hotfix_tables();
		for (i=0; i < hotfix_table_count; i++)
		{
		       LONGLONG size;
		       nwvp_rpartition_info rpart_info;

		       if (hotfix_table[i].active_flag != 0)
			  sprintf(displayBuffer,
				  "%2d  %2d  %2d  %2d",
				  (int) i,
				  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
				  (int)hotfix_table[i].mirror_count,
				  (int) hotfix_table[i].segment_count);
		       else
			  sprintf(displayBuffer,
				  "%2d  %2d  ??  %2d",
				  (int) i,
				  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
				  (int) hotfix_table[i].segment_count);
		       write_portal(mportal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		       size = (LONGLONG)((LONGLONG)hotfix_table[i].logical_block_count *
					       (LONGLONG)IO_BLOCK_SIZE);
		       if (size < 0x100000)
			  sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
		       else
			  sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
		       write_portal(mportal, displayBuffer, i, 16, BRITEWHITE | BGBLUE);

		       size = (LONGLONG)((LONGLONG)hotfix_table[i].hotfix_block_count *
					       (LONGLONG)IO_BLOCK_SIZE);

		       if (hotfix_table[i].hotfix_block_count != 0)
		       {
			  if (size < 0x100000)
			     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			  else
			     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);

			  size = (LONGLONG)((LONGLONG)hotfix_table[i].physical_block_count *
						       (LONGLONG)IO_BLOCK_SIZE);
			  if (size < 0x100000)
			     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			  else
			     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			  write_portal(mportal, displayBuffer, i, 36, BRITEWHITE | BGBLUE);

		          sprintf(displayBuffer,
			       "%s%s",
			       (hotfix_table[i].insynch_flag != 0)
			       ? "IN/" : "OUT/",
			       (hotfix_table[i].active_flag != 0)
			       ? "YES" : "NO");
		          write_portal(mportal, displayBuffer, i, 48, BRITEWHITE | BGBLUE);

		          nwvp_rpartition_return_info(hotfix_table[i].rpart_handle, &rpart_info);
		          if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
		          {
#if (LINUX_UTIL)
		             sprintf(displayBuffer, "%d:%d %s",
			          (int)rpart_info.physical_disk_number,
			          (int)rpart_info.partition_number + 1,
			          PhysicalDiskNames[rpart_info.physical_disk_number % MaxHandlesSupported]);
		             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#else
		             sprintf(displayBuffer, "%d:%d",
			          (int)rpart_info.physical_disk_number,
			          (int)rpart_info.partition_number + 1);
		             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#endif
		          }
		       }
		       else
		       {
			  sprintf(displayBuffer, "     PARTITION NOT PRESENT ");
			  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);
		       }
		}

		update_static_portal(mportal);

		build_segment_tables();
		clear_portal(portal);
		for (i=0; (i < segment_table_count) && (i < 256); i++)
		{
		   LONGLONG size;

		   if (segment_mark_table[i] == 0)
		      sprintf(displayBuffer, "     %2d ", (int) i);
		   else
		      sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
		   write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		   if (segment_table[i].free_flag == 0)
		   {
		      sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF,
			  &segment_table[i].VolumeName[1],
			  (int) segment_table[i].segment_number);
		      write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

		      if ((int) segment_table[i].total_segments < 10)
			 sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
		      else
			 sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
		      write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
		   }
		   else
		   {
		      sprintf(displayBuffer, "   %4d      *** FREE ***              ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF);
		      write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
		   }

		   size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
			  (LONGLONG)IO_BLOCK_SIZE);

		   if (size < 0x100000)
		      sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		   else
		      sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		   write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

		   size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
			       (LONGLONG)IO_BLOCK_SIZE);
		   if (size < 0x100000)
		      sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		   else
		      sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		   write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
		}
		return 0;
	     }
	  }
	  error_portal("****  Invalid Hot Fix Size ****", 10);
	  return 0;
       }
       error_portal("****  Cannot Resize Partition With NetWare Volumes ****", 10);
       return 0;
    }
    error_portal("****  Invalid Partition Number ****", 10);
    return 0;
}

ULONG mirrorHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    ULONG help, i, index1, iportal, ccode, message;
    BYTE primary[6];
    BYTE secondary[6];
    BYTE displayBuffer[255];

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  help = make_portal(&ConsoleScreen,
		       " Help for Volume Mirroring Configuration ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       256,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(MirroringHelp) / sizeof (BYTE *); i++)
	     write_portal_cleol(help, MirroringHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Menu ENTER-EditHotFix INS-AddMirror DEL-DelMirror SPACE-ActivateGroup");
#else
	  sprintf(displayBuffer, "  ESC-Menu ENTER-EditHotFix INS-AddMirror DEL-DelMirror SPACE-ActivateGroup");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       case INS:
	  NWFSSet(primary, 0, 6);
	  NWFSSet(secondary, 0, 6);
	  iportal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       10,
		       5,
		       16,
		       75,
		       5,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	  if (!iportal)
	     break;

	  activate_static_portal(iportal);

	  ccode = add_field_to_portal(iportal, 1, 2, WHITE | BGBLUE,
			 "Enter Primary Partition Number:",
			 strlen("Enter Primary Partition Number:"),
			 primary, sizeof(primary),
			 0, 0, 0, 0, FIELD_ENTRY);
	  if (ccode)
	  {
	     error_portal("****  error:  could not allocate memory ****", 10);
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     break;
	  }

	  ccode = add_field_to_portal(iportal, 3, 2, WHITE | BGBLUE,
			 "Enter Secondary Partition Number:",
			 strlen("Enter Secondary Partition Number:"),
			 secondary, sizeof(secondary),
			 0, 0, 0, 0, FIELD_ENTRY);
	  if (ccode)
	  {
	     error_portal("****  error:  could not allocate memory ****", 10);
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     break;
	  }

	  ccode = input_portal_fields(iportal);
	  if (ccode)
	  {
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     break;
	  }

	  if (iportal)
	  {
	     deactivate_static_portal(iportal);
	     free_portal(iportal);
	  }

	  if (!primary[0])
	  {
	     error_portal("**** Invalid Primary Partition Number ****", 10);
	     break;
	  }

	  if (!secondary[0])
	  {
	     error_portal("**** Invalid Secondary Partition Number ****", 10);
	     break;
	  }

	  index = atol(primary);
	  index1 = atol(secondary);

	  if (index == index1)
	  {
	     error_portal("**** Partition Cannot be Mirrored To Itself ****", 10);
	     break;
	  }

	  if (index < hotfix_table_count)
	  {
	     if (index1 < hotfix_table_count)
	     {
		if (hotfix_table[index1].mirror_count == 1)
		{
		   if (hotfix_table[index1].segment_count == 0)
		   {
		      if ((hotfix_table[index1].physical_block_count + 65) >= hotfix_table[index].logical_block_count)
		      {
			 message = create_message_portal("Adding Mirror ...",
							  10, YELLOW | BGBLUE);

			 nwvp_lpartition_unformat(hotfix_table[index1].lpart_handle);
			 nwvp_partition_add_mirror(hotfix_table[index].lpart_handle, hotfix_table[index1].rpart_handle);

			 if (message)
			    close_message_portal(message);

			 SyncDisks();

			 nwconfig_build_hotfix_tables();
			 for (i=0; i < hotfix_table_count; i++)
			 {
			       LONGLONG size;
			       nwvp_rpartition_info rpart_info;

			       if (hotfix_table[i].active_flag != 0)
				  sprintf(displayBuffer,
					  "%2d  %2d  %2d  %2d",
					  (int) i,
				          (int) (hotfix_table[i].lpart_handle & 0xFFFF),
					  (int)hotfix_table[i].mirror_count,
					  (int) hotfix_table[i].segment_count);
			       else
				  sprintf(displayBuffer,
					  "%2d  %2d  ??  %2d",
					  (int) i,
				          (int) (hotfix_table[i].lpart_handle & 0xFFFF),
					  (int) hotfix_table[i].segment_count);
			       write_portal(mportal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

			       size = (LONGLONG)((LONGLONG)hotfix_table[i].logical_block_count *
						       (LONGLONG)IO_BLOCK_SIZE);
			       if (size < 0x100000)
				  sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			       else
				  sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			       write_portal(mportal, displayBuffer, i, 16, BRITEWHITE | BGBLUE);

			       size = (LONGLONG)((LONGLONG)hotfix_table[i].hotfix_block_count *
						       (LONGLONG)IO_BLOCK_SIZE);

			       if (hotfix_table[i].hotfix_block_count != 0)
			       {
				  if (size < 0x100000)
				     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
				  else
				     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
				  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);

				  size = (LONGLONG)((LONGLONG)hotfix_table[i].physical_block_count *
							       (LONGLONG)IO_BLOCK_SIZE);
				  if (size < 0x100000)
				     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
				  else
				     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
				  write_portal(mportal, displayBuffer, i, 36, BRITEWHITE | BGBLUE);

			          sprintf(displayBuffer,
				       "%s%s",
				       (hotfix_table[i].insynch_flag != 0)
				       ? "IN/" : "OUT/",
				       (hotfix_table[i].active_flag != 0)
				       ? "YES" : "NO");
			          write_portal(mportal, displayBuffer, i, 48, BRITEWHITE | BGBLUE);

			          nwvp_rpartition_return_info(hotfix_table[i].rpart_handle, &rpart_info);
			          if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
			          {
#if (LINUX_UTIL)
			             sprintf(displayBuffer, "%d:%d %s",
				          (int)rpart_info.physical_disk_number,
				          (int)rpart_info.partition_number + 1,
				          PhysicalDiskNames[rpart_info.physical_disk_number % MaxHandlesSupported]);
			             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#else
			             sprintf(displayBuffer, "%d:%d",
				          (int)rpart_info.physical_disk_number,
				          (int)rpart_info.partition_number + 1);
			             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#endif
			          }
			       }
			       else
			       {
				  sprintf(displayBuffer, "     PARTITION NOT PRESENT ");
				  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);
			       }
			 }
			 update_static_portal(mportal);
			 break;
		      }
		      error_portal("**** Secondary Partition Too Small ****", 10);
		      break;
		   }
		   error_portal("**** Secondary Partition Has Volume Segments ****", 10);
		   break;
		}
		error_portal("**** Secondary Partition Is Already Mirrored ****", 10);
		break;
	     }
	     error_portal("**** Invalid Secondary Partition Number ****", 10);
	     break;
	  }
	  error_portal("**** Invalid Primary Partition Number ****", 10);
	  break;

       case DEL:
	  NWFSSet(primary, 0, 6);
	  NWFSSet(secondary, 0, 6);

	  sprintf(displayBuffer, "  Unmirror Partition %u ?",
		 (unsigned)index);
	  ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	  if (!ccode)
	     break;

	  if ((index < hotfix_table_count) && (hotfix_table[index].mirror_count > 1))
	  {
	     i = 0;
	     if (hotfix_table[index].insynch_flag != 0)
	     {
		for (i=0; i < hotfix_table_count; i++)
		{
		   if ((i != index) &&
		       (hotfix_table[i].lpart_handle == hotfix_table[index].lpart_handle) &&
		       (hotfix_table[i].insynch_flag != 0))
		      break;
		}
	     }

	     if (i != hotfix_table_count)
	     {
		message = create_message_portal("Removing Mirror ...",
						10, YELLOW | BGBLUE);
		if (nwvp_partition_del_mirror(hotfix_table[index].lpart_handle, hotfix_table[index].rpart_handle) == 0)
		{
		   if (message)
		      close_message_portal(message);

		   SyncDisks();
		   clear_portal(mportal);

		   nwconfig_build_hotfix_tables();
		   for (i=0; i < hotfix_table_count; i++)
		   {
		       LONGLONG size;
		       nwvp_rpartition_info rpart_info;

		       if (hotfix_table[i].active_flag != 0)
			  sprintf(displayBuffer,
				  "%2d  %2d  %2d  %2d",
				  (int) i,
				  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
				  (int)hotfix_table[i].mirror_count,
				  (int) hotfix_table[i].segment_count);
		       else
			  sprintf(displayBuffer,
				  "%2d  %2d  ??  %2d",
				  (int) i,
				  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
				  (int) hotfix_table[i].segment_count);
		       write_portal(mportal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		       size = (LONGLONG)((LONGLONG)hotfix_table[i].logical_block_count *
					       (LONGLONG)IO_BLOCK_SIZE);
		       if (size < 0x100000)
			  sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
		       else
			  sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
		       write_portal(mportal, displayBuffer, i, 16, BRITEWHITE | BGBLUE);

		       size = (LONGLONG)((LONGLONG)hotfix_table[i].hotfix_block_count *
					       (LONGLONG)IO_BLOCK_SIZE);

		       if (hotfix_table[i].hotfix_block_count != 0)
		       {
			  if (size < 0x100000)
			     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			  else
			     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);

			  size = (LONGLONG)((LONGLONG)hotfix_table[i].physical_block_count *
						       (LONGLONG)IO_BLOCK_SIZE);
			  if (size < 0x100000)
			     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			  else
			     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			  write_portal(mportal, displayBuffer, i, 36, BRITEWHITE | BGBLUE);

		          sprintf(displayBuffer,
			       "%s%s",
			       (hotfix_table[i].insynch_flag != 0)
			       ? "IN/" : "OUT/",
			       (hotfix_table[i].active_flag != 0)
			       ? "YES" : "NO");
		          write_portal(mportal, displayBuffer, i, 48, BRITEWHITE | BGBLUE);

		          nwvp_rpartition_return_info(hotfix_table[i].rpart_handle, &rpart_info);
		          if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
		          {
#if (LINUX_UTIL)
		             sprintf(displayBuffer, "%d:%d %s",
			          (int)rpart_info.physical_disk_number,
			          (int)rpart_info.partition_number + 1,
			          PhysicalDiskNames[rpart_info.physical_disk_number % MaxHandlesSupported]);
		             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#else
		             sprintf(displayBuffer, "%d:%d",
			          (int)rpart_info.physical_disk_number,
			          (int)rpart_info.partition_number + 1);
		             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#endif
		          }
		       }
		       else
		       {
			  sprintf(displayBuffer, "     PARTITION NOT PRESENT ");
			  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);
		       }

		   }
		   update_static_portal(mportal);
		   break;
		}
		if (message)
		   close_message_portal(message);
	     }
	     error_portal("**** Can not delete last valid mirror member ****", 10);
	     break;
	  }
	  error_portal("**** Invalid Primary Partition Number ****", 10);
	  break;

       case SPACE:
	  NWFSSet(primary, 0, 6);
	  NWFSSet(secondary, 0, 6);

	  sprintf(displayBuffer, "  Activate Mirror Group For Partition %u ?",
		 (unsigned)index);
	  ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	  if (!ccode)
	     break;

	  if (index < hotfix_table_count)
	  {
	     if (hotfix_table[index].active_flag == 0)
	     {
		message = create_message_portal("Activating Group ...",
						10, YELLOW | BGBLUE);
		if (nwvp_lpartition_activate(hotfix_table[index].lpart_handle) == 0)
		{
		   if (message)
		      close_message_portal(message);

		   SyncDisks();

		   nwconfig_build_hotfix_tables();
		   for (i=0; i < hotfix_table_count; i++)
		   {
		       LONGLONG size;
		       nwvp_rpartition_info rpart_info;

		       if (hotfix_table[i].active_flag != 0)
			  sprintf(displayBuffer,
				  "%2d  %2d  %2d  %2d",
				  (int) i,
				  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
				  (int)hotfix_table[i].mirror_count,
				  (int) hotfix_table[i].segment_count);
		       else
			  sprintf(displayBuffer,
				  "%2d  %2d  ??  %2d",
				  (int) i,
				  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
				  (int) hotfix_table[i].segment_count);
		       write_portal(mportal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		       size = (LONGLONG)((LONGLONG)hotfix_table[i].logical_block_count *
					       (LONGLONG)IO_BLOCK_SIZE);
		       if (size < 0x100000)
			  sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
		       else
			  sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
		       write_portal(mportal, displayBuffer, i, 16, BRITEWHITE | BGBLUE);

		       size = (LONGLONG)((LONGLONG)hotfix_table[i].hotfix_block_count *
					       (LONGLONG)IO_BLOCK_SIZE);

		       if (hotfix_table[i].hotfix_block_count != 0)
		       {
			  if (size < 0x100000)
			     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			  else
			     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);

			  size = (LONGLONG)((LONGLONG)hotfix_table[i].physical_block_count *
						       (LONGLONG)IO_BLOCK_SIZE);
			  if (size < 0x100000)
			     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
			  else
			     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
			  write_portal(mportal, displayBuffer, i, 36, BRITEWHITE | BGBLUE);

		          sprintf(displayBuffer,
			       "%s%s",
			       (hotfix_table[i].insynch_flag != 0)
			       ? "IN/" : "OUT/",
			       (hotfix_table[i].active_flag != 0)
			       ? "YES" : "NO");
		          write_portal(mportal, displayBuffer, i, 48, BRITEWHITE | BGBLUE);

		          nwvp_rpartition_return_info(hotfix_table[i].rpart_handle, &rpart_info);
		          if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
		          {
#if (LINUX_UTIL)
		             sprintf(displayBuffer, "%d:%d %s",
			          (int)rpart_info.physical_disk_number,
			          (int)rpart_info.partition_number + 1,
			          PhysicalDiskNames[rpart_info.physical_disk_number % MaxHandlesSupported]);
		             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#else
		             sprintf(displayBuffer, "%d:%d",
			          (int)rpart_info.physical_disk_number,
			          (int)rpart_info.partition_number + 1);
		             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#endif
		          }
		       }
		       else
		       {
			  sprintf(displayBuffer, "     PARTITION NOT PRESENT ");
			  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);
		       }

		   }
		   update_static_portal(mportal);
		   break;
		}
		if (message)
		   close_message_portal(message);

		error_portal("**** Can not Activate Logical Partition ****", 10);
		break;
	     }
	     error_portal("**** Logical Partition Already Active ****", 10);
	     break;
	  }
	  error_portal("**** Invalid Partition Number ****", 10);
	  break;

       default:
	  break;
    }

    SyncDisks();
    build_segment_tables();
    clear_portal(portal);
    for (i=0; (i < segment_table_count) && (i < 256); i++)
    {
       LONGLONG size;

       if (segment_mark_table[i] == 0)
	  sprintf(displayBuffer, "     %2d ", (int) i);
       else
	  sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
       write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

       if (segment_table[i].free_flag == 0)
       {
	  sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
		  (int) segment_table[i].lpart_handle & 0x0000FFFF,
		  &segment_table[i].VolumeName[1],
		  (int) segment_table[i].segment_number);
	  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

	  if ((int) segment_table[i].total_segments < 10)
	     sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
	  else
	     sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
	  write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
       }
       else
       {
	  sprintf(displayBuffer, "   %4d      *** FREE ***              ",
		  (int) segment_table[i].lpart_handle & 0x0000FFFF);
	  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
       }

       size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
		 (LONGLONG)IO_BLOCK_SIZE);

       if (size < 0x100000)
	  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

       size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
		       (LONGLONG)IO_BLOCK_SIZE);
       if (size < 0x100000)
	  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
    }
    return 0;
}

void NWConfigEditHotFix(void)
{
    ULONG i;
    BYTE displayBuffer[255] = { "" };

    SyncDisks();

    mportal = make_portal(&ConsoleScreen,
		       "     Mirror  Vol      Data     Hotfix     Part   Sync/",
		       "  # Grp  Cnt Segs     Size      Size      Size   Active  Disk:Partition",
		       3,
		       0,
		       ConsoleScreen.nLines - 2,
		       ConsoleScreen.nCols - 1,
		       MAX_PARTITIONS,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       mirrorFunction,
		       0,
		       mirrorHandler,
		       FALSE);
    if (!mportal)
       return;

    nwconfig_build_hotfix_tables();
    for (i=0; i < hotfix_table_count; i++)
    {
       LONGLONG size;
       nwvp_rpartition_info rpart_info;

       if (hotfix_table[i].active_flag != 0)
	  sprintf(displayBuffer,
		  "%2d  %2d  %2d  %2d",
		  (int) i,
		  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
		  (int)hotfix_table[i].mirror_count,
		  (int) hotfix_table[i].segment_count);
       else
	  sprintf(displayBuffer,
		  "%2d  %2d  ??  %2d",
		  (int) i,
		  (int) (hotfix_table[i].lpart_handle & 0xFFFF),
		  (int) hotfix_table[i].segment_count);
       write_portal(mportal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

       size = (LONGLONG)((LONGLONG)hotfix_table[i].logical_block_count *
			       (LONGLONG)IO_BLOCK_SIZE);
       if (size < 0x100000)
	  sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(mportal, displayBuffer, i, 16, BRITEWHITE | BGBLUE);

       size = (LONGLONG)((LONGLONG)hotfix_table[i].hotfix_block_count *
			       (LONGLONG)IO_BLOCK_SIZE);

       if (hotfix_table[i].hotfix_block_count != 0)
       {
	  if (size < 0x100000)
	     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
	  else
	     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
	  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);

	  size = (LONGLONG)((LONGLONG)hotfix_table[i].physical_block_count *
				       (LONGLONG)IO_BLOCK_SIZE);
	  if (size < 0x100000)
	     sprintf(displayBuffer, "%8uK", (unsigned)(size / (LONGLONG)1024));
	  else
	     sprintf(displayBuffer, "%8uMB", (unsigned)(size / (LONGLONG)0x100000));
	  write_portal(mportal, displayBuffer, i, 36, BRITEWHITE | BGBLUE);

          sprintf(displayBuffer,
	       "%s%s",
	       (hotfix_table[i].insynch_flag != 0)
	       ? "IN/" : "OUT/",
	       (hotfix_table[i].active_flag != 0)
	       ? "YES" : "NO");
          write_portal(mportal, displayBuffer, i, 48, BRITEWHITE | BGBLUE);

          nwvp_rpartition_return_info(hotfix_table[i].rpart_handle, &rpart_info);
          if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
          {
#if (LINUX_UTIL)
             sprintf(displayBuffer, "%d:%d %s",
	          (int)rpart_info.physical_disk_number,
	          (int)rpart_info.partition_number + 1,
	          PhysicalDiskNames[rpart_info.physical_disk_number % MaxHandlesSupported]);
             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#else
             sprintf(displayBuffer, "%d:%d",
	          (int)rpart_info.physical_disk_number,
	          (int)rpart_info.partition_number + 1);
             write_portal(mportal, displayBuffer, i, 56, BRITEWHITE | BGBLUE);
#endif
          }
       }
       else
       {
	  sprintf(displayBuffer, "     PARTITION NOT PRESENT ");
	  write_portal(mportal, displayBuffer, i, 26, BRITEWHITE | BGBLUE);
       }

    }

    activate_portal(mportal);

    if (mportal)
    {
       deactivate_static_portal(mportal);
       free_portal(mportal);
    }

    return;

}


ULONG menuFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG menu_index)
{
    ULONG j, ccode, vol, i, k;
    ULONG vpart_handle, volmenu;
    ULONG index, message;
    ULONG cluster_size;
    segment_info stable[32];
    vpartition_info vpart_info;
    nwvp_vpartition_info vpinfo;
    BYTE volnameBuffer[16] = { "" };
    BYTE sizeBuffer[16] = { "" };
    BYTE vtBuffer[10] = { "" };
    BYTE saBuffer[6] = { "" };
    BYTE fcBuffer[6] = { "" };
    BYTE vaBuffer[6] = { "" };
    BYTE dmBuffer[6] = { "" };
    ULONG vtFlag, saFlag, fcFlag, ndsFlag, dmFlag, adFlag;
    BYTE displayBuffer[255];
    BYTE *bptr;
    ULONG cluster_sel = 4;

    switch (value)
    {
      case 1:
	 vtFlag = ndsFlag = dmFlag = adFlag = TRUE;
	 saFlag = fcFlag = 0;
	 for (j=0; j < segment_table_count; j++)
	 {
	    if (segment_mark_table[j] == 1)
	       break;
	 }

	 if (j == segment_table_count)
	 {
	    error_portal("****  No Marked Segments ****", 7);
	    return 0;
	 }

	 NWFSSet(volnameBuffer, 0, 16);
	 NWFSSet(sizeBuffer, 0 ,16);
	 NWFSSet(saBuffer, 0, 6);
	 NWFSSet(fcBuffer, 0, 6);
	 NWFSSet(vaBuffer, 0, 6);
	 NWFSSet(dmBuffer, 0, 6);
	 NWFSSet(displayBuffer, 0, 255);

	 vol = make_portal(&ConsoleScreen,
		       0,
		       0,
		       7,
		       8,
		       20,
		       72,
		       12,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	 if (!vol)
	    return 0;

#if (LINUX_UTIL)
	 sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-Accept  F3-Back");
	 write_portal_cleol(vol, displayBuffer, 11, 0, BLUE | BGWHITE);
#else
	 sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-Accept  ESC-Back");
	 write_portal_cleol(vol, displayBuffer, 11, 0, BLUE | BGWHITE);
#endif

	 activate_static_portal(vol);

	 ccode = add_field_to_portal(vol, 1, 2, WHITE | BGBLUE,
				     "Enter New Volume Name:      ",
			      strlen("Enter New Volume Name:      "),
			      volnameBuffer,
			      sizeof(volnameBuffer),
			      0, 0, 0, 0, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = add_field_to_portal(vol, 3, 2, WHITE | BGBLUE,
				     "Volume Block Size (Kbytes): ",
			      strlen("Volume Block Size (Kbytes): "),
			      sizeBuffer,
			      sizeof(sizeBuffer),
			      BlockSizes, 5, 4, &cluster_sel, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = add_field_to_portal(vol, 4, 2, WHITE | BGBLUE,
				     "Volume Version:             ",
			      strlen("Volume Version:             "),
			      vtBuffer,
			      sizeof(vtBuffer),
			      VolumeType, 2, 1, &vtFlag, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = add_field_to_portal(vol, 6, 2, WHITE | BGBLUE,
				     "Volume Suballocation:       ",
			      strlen("Volume Suballocation:       "),
			      saBuffer,
			      sizeof(saBuffer),
			      OnOff, 2, 0, &saFlag, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = add_field_to_portal(vol, 7, 2, WHITE | BGBLUE,
				     "Volume Compression:         ",
			      strlen("Volume Compression:         "),
			      fcBuffer,
			      sizeof(fcBuffer),
			      OnOff, 2, 0, &fcFlag, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = add_field_to_portal(vol, 8, 2, WHITE | BGBLUE,
				     "Volume Auditing:            ",
			      strlen("Volume Auditing:            "),
			      vaBuffer,
			      sizeof(vaBuffer),
			      OnOff, 2, 1, &adFlag, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = add_field_to_portal(vol, 9, 2, WHITE | BGBLUE,
				     "Volume Data Migration:      ",
			      strlen("Volume Data Migration:      "),
			      dmBuffer,
			      sizeof(dmBuffer),
			      OnOff, 2, 1, &dmFlag, 0);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 ccode = input_portal_fields(vol);
	 if (ccode)
	 {
	    if (vol)
	    {
	       deactivate_static_portal(vol);
	       free_portal(vol);
	    }
	    return 0;
	 }

	 if (vol)
	 {
	    deactivate_static_portal(vol);
	    free_portal(vol);
	 }

	 for (j=0; j < 16; j++)
	    vpart_info.volume_name[j] = 0;

	 bptr = volnameBuffer;
	 while (bptr[0] == ' ')
	    bptr ++;

	 while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
	 {
	    if ((bptr[0] >= 'a') && (bptr[0] <= 'z'))
	       bptr[0] = (bptr[0] - 'a') + 'A';
	    vpart_info.volume_name[vpart_info.volume_name[0] + 1] = bptr[0];
	    vpart_info.volume_name[0] ++;
	    if (vpart_info.volume_name[0] == 15)
	       break;
	    bptr ++;
	 }

	 if (vpart_info.volume_name[0] != 0)
	 {
	    if (convert_volume_name_to_handle(&vpart_info.volume_name[0], &vpart_handle) == 0)
	    {
	       if ((nwvp_vpartition_return_info(vpart_handle, &vpinfo) == 0) &&
			(vpinfo.volume_valid_flag != 0))
	       {
		  error_portal("****  Can not expand incomplete volume ****", 7);
		  return 0;
	       }

	       sprintf(displayBuffer,
		       "  Volume %s Exists.  Expand Volume ? (Y/N): ",
		       &vpart_info.volume_name[1]);

	       ccode = confirm_menu(displayBuffer, 11, YELLOW | BGBLUE);
	       if (ccode)
	       {
		  message = create_message_portal("Extending Volume ...",
						  10,
						  YELLOW | BGBLUE);
		  index = 1;
		  for (j=0; j < segment_table_count; j++)
		  {
		     if (segment_mark_table[j] == index)
		     {
			stable[index - 1].lpartition_id = segment_table[j].lpart_handle;
			stable[index - 1].block_offset = segment_table[j].segment_offset;
			stable[index - 1].block_count = segment_table[j].segment_count;

			if (nwvp_vpartition_add_segment(vpart_handle, &stable[index-1]) != 0)
			{
			   build_segment_tables();
			   error_portal("****  error adding volume segments ****", 7);
			   return 0;
			}
			index ++;
			j = 0xFFFFFFFF;
		     }
		  }

		  if (message)
		     close_message_portal(message);

		  build_segment_tables();
		  clear_portal(portal);
		  for (i=0; (i < segment_table_count) && (i < 256); i++)
		  {
		     LONGLONG size;

		     if (segment_mark_table[i] == 0)
			sprintf(displayBuffer, "     %2d ", (int) i);
		     else
			sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
		     write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		     if (segment_table[i].free_flag == 0)
		     {
			sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
				  (int) segment_table[i].lpart_handle & 0x0000FFFF,
				  &segment_table[i].VolumeName[1],
				  (int) segment_table[i].segment_number);
			write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

			if ((int) segment_table[i].total_segments < 10)
			   sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
			else
			   sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
			write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
		     }
		     else
		     {
			sprintf(displayBuffer, "   %4d      *** FREE ***              ",
				  (int) segment_table[i].lpart_handle & 0x0000FFFF);
			write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
		     }

		     size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
			       (LONGLONG)IO_BLOCK_SIZE);

		     if (size < 0x100000)
			sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		     else
			sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		     write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

		     size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
				       (LONGLONG)IO_BLOCK_SIZE);
		     if (size < 0x100000)
			sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		     else
			sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		     write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
		  }
		  update_static_portal(portal);
		  break;
	       }
	       break;
	    }

	    switch (cluster_sel)
	    {
	       case 0:
		  cluster_size = 4;
		  break;

	       case 1:
		  cluster_size = 8;
		  break;

	       case 2:
		  cluster_size = 16;
		  break;

	       case 3:
		  cluster_size = 32;
		  break;

	       case 4:
		  cluster_size = 64;
		  break;

	       default:
		  cluster_size = 0;
		  break;

	    }

	    if (cluster_size)
	    {
	       vpart_info.cluster_count = 0;
	       if (cluster_size == 64)
		      vpart_info.blocks_per_cluster = 0x10;
	       else
	       if (cluster_size == 32)
		      vpart_info.blocks_per_cluster = 0x8;
	       else
	       if (cluster_size == 16)
		      vpart_info.blocks_per_cluster = 0x4;
	       else
	       if (cluster_size == 8)
		      vpart_info.blocks_per_cluster = 0x2;
	       else
	       if (cluster_size == 4)
		      vpart_info.blocks_per_cluster = 1;
	       else
	       {
		  error_portal("****  invalid cluster size 64K 32K 16K 8K 4K ****", 7);
		  return 0;
	       }

	       index = 1;
	       for (j=0; j < segment_table_count; j++)
	       {
		  if (segment_mark_table[j] == index)
		  {
		     stable[index - 1].lpartition_id = segment_table[j].lpart_handle;
		     stable[index - 1].block_offset = segment_table[j].segment_offset;
		     stable[index - 1].block_count = segment_table[j].segment_count;
		     stable[index - 1].extra = 0;
		     vpart_info.cluster_count += segment_table[j].segment_count / vpart_info.blocks_per_cluster;
		     index ++;
		     j = 0xFFFFFFFF;
		  }
	       }

	       vpart_info.segment_count = (index - 1);
	       vpart_info.flags = 0;

	       // if a 4.x/5.x volume is specified
	       if (vtFlag)
	       {
		  vpart_info.flags |= NEW_TRUSTEE_COUNT;
		  if (!saFlag)
                  {
		     vpart_info.flags |= SUB_ALLOCATION_ON;
                  }
		  if (!fcFlag)
                  {
		     vpart_info.flags |= FILE_COMPRESSION_ON;
                  }
		  vpart_info.flags |= NDS_FLAG;
	       }
	       else
		  vpart_info.flags |= 0x80000000;

	       if (!dmFlag)
		  vpart_info.flags |= DATA_MIGRATION_ON;
	       if (!adFlag)
		  vpart_info.flags |= AUDITING_ON;

	       message = create_message_portal("Creating Volume ...", 10,
					       YELLOW | BGBLUE);

	       if (nwvp_vpartition_format(&vpart_handle, &vpart_info, &stable[0]) == 0)
	       {
		  if (message)
		     close_message_portal(message);

		  ScanVolumes();
		  build_segment_tables();
		  clear_portal(portal);
		  for (i=0; (i < segment_table_count) && (i < 256); i++)
		  {
		     LONGLONG size;

		     if (segment_mark_table[i] == 0)
			sprintf(displayBuffer, "     %2d ", (int) i);
		     else
			sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
		     write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		     if (segment_table[i].free_flag == 0)
		     {
			sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
				  (int) segment_table[i].lpart_handle & 0x0000FFFF,
				  &segment_table[i].VolumeName[1],
				  (int) segment_table[i].segment_number);
			write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

			if ((int) segment_table[i].total_segments < 10)
			   sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
			else
			   sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
			write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
		     }
		     else
		     {
			sprintf(displayBuffer, "   %4d      *** FREE ***              ",
				  (int) segment_table[i].lpart_handle & 0x0000FFFF);
			write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
		     }

		     size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
			       (LONGLONG)IO_BLOCK_SIZE);

		     if (size < 0x100000)
			sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		     else
			sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		     write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

		     size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
				       (LONGLONG)IO_BLOCK_SIZE);
		     if (size < 0x100000)
			sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		     else
			sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		     write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
		  }
		  update_static_portal(portal);
		  return 0;
	       }
	       if (message)
		  close_message_portal(message);
	       error_portal("****  Volume Creation Failure ****", 7);
	       return 0;
	    }
	    error_portal("****  Invalid Cluster Size ****", 7);
	    return 0;
	 }
	 error_portal("**** Invalid Volume Name ****", 7);
	 return 0;

      case 2:
	 volmenu = make_menu(&ConsoleScreen,
			  " Select a Volume To Delete ",
			  8,
			  25,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  MAX_VOLUMES);
	 if (!volmenu)
	    break;

	 build_volume_tables();
	 build_segment_tables();
	 for (j=0; j < segment_table_count; j++)
	 {
	    if (segment_table[j].free_flag == 0)
	    {
	       for (k=0; k < j; k++)
	       {
		  if (segment_table[k].free_flag == 0)
		  {
		     if (NWFSCompare(&segment_table[j].VolumeName[0],
				     &segment_table[k].VolumeName[0], 16) == 0)
			break;
		  }
	       }
	       if (k == j)
	       {
		  add_item_to_menu(volmenu, &segment_table[j].VolumeName[1], j);
	       }
	    }
	 }

	 ccode = activate_menu(volmenu);

	 if (volmenu)
	    free_menu(volmenu);

	 if (ccode == (ULONG) -1)
	    break;

	 for (j=0; j < 16; j++)
	    vpart_info.volume_name[j] = 0;

	 bptr = &segment_table[ccode].VolumeName[1];
	 while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
	 {
	    vpart_info.volume_name[vpart_info.volume_name[0] + 1] = bptr[0];
	    vpart_info.volume_name[0] ++;

	    if (vpart_info.volume_name[0] == 15)
	       break;
	    bptr++;
	 }

	 if (vpart_info.volume_name[0] != 0)
	 {
	    if (convert_volume_name_to_handle(&vpart_info.volume_name[0], &vpart_handle) == 0)
	    {
	       sprintf(displayBuffer, " Delete Volume %s ? ",
		       &vpart_info.volume_name[1]);

	       ccode = confirm_menu(displayBuffer, 11, YELLOW | BGBLUE);
	       if (!ccode)
		  return 0;

	       if (nwvp_vpartition_unformat(vpart_handle) == 0)
	       {
		  ScanVolumes();
		  build_volume_tables();
		  build_segment_tables();
		  clear_portal(portal);

		  for (i=0; (i < segment_table_count) && (i < 256); i++)
		  {
		     LONGLONG size;

		     if (segment_mark_table[i] == 0)
			sprintf(displayBuffer, "     %2d ", (int) i);
		     else
			sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
		     write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		     if (segment_table[i].free_flag == 0)
		     {
			sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
			      (int) segment_table[i].lpart_handle & 0x0000FFFF,
			      &segment_table[i].VolumeName[1],
			      (int) segment_table[i].segment_number);
			write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

			if ((int) segment_table[i].total_segments < 10)
			   sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
			else
			   sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
			write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
		     }
		     else
		     {
			sprintf(displayBuffer, "   %4d      *** FREE ***              ",
			      (int) segment_table[i].lpart_handle & 0x0000FFFF);
			write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
		     }

		     size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
				       (LONGLONG)IO_BLOCK_SIZE);

		     if (size < 0x100000)
			sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		     else
			sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		     write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

		     size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
				       (LONGLONG)IO_BLOCK_SIZE);
		     if (size < 0x100000)
			sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		     else
			sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		     write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
		  }
		  update_static_portal(portal);
		  return 0;
	       }
	    }
	 }
	 error_portal("**** Invalid Volume Name - Press <Return> ****", 7);
	 break;

      case 3:
#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Menu");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Menu");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	 NWConfigEditPartitions();

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Segments");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Segments");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;

      case 4:
#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F3-Menu ENTER-EditHotFix INS-AddMirror DEL-DelMirror SPACE-ActivateGroup");
#else
	 sprintf(displayBuffer, "  ESC-Menu ENTER-EditHotFix INS-AddMirror DEL-DelMirror SPACE-ActivateGroup");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	 NWConfigEditHotFix();

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Segments");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Segments");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;

      case 5:
#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F3-Menu  INS-Add Namespace  DEL-Delete Namespace  ENTER-Change Flags");
#else
	 sprintf(displayBuffer, "  ESC-Menu  INS-Add Namespace  DEL-Delete Namespace  ENTER-Change Flags");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	 NWConfigEditNamespaces();

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Segments");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Segments");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;
    }

    ScanVolumes();
    build_volume_tables();
    build_segment_tables();
    clear_portal(portal);

    for (i=0; (i < segment_table_count) && (i < 256); i++)
    {
       LONGLONG size;

       if (segment_mark_table[i] == 0)
	  sprintf(displayBuffer, "     %2d ", (int) i);
       else
	  sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
       write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

       if (segment_table[i].free_flag == 0)
       {
	  sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
		  (int) segment_table[i].lpart_handle & 0x0000FFFF,
		  &segment_table[i].VolumeName[1],
		  (int) segment_table[i].segment_number);
	  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

	  if ((int) segment_table[i].total_segments < 10)
	     sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
	  else
	     sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
	  write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
       }
       else
       {
	  sprintf(displayBuffer, "   %4d      *** FREE ***              ",
		  (int) segment_table[i].lpart_handle & 0x0000FFFF);
	  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
       }

       size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
		 (LONGLONG)IO_BLOCK_SIZE);

       if (size < 0x100000)
	  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

       size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
		       (LONGLONG)IO_BLOCK_SIZE);
       if (size < 0x100000)
	  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
    }
    update_static_portal(portal);
    return 0;
}

ULONG segmentFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
    if (index < segment_table_count)
    {
       if (segment_table[index].free_flag != 0)
       {
	  register ULONG SegmentSize, block_count, j, i, iportal, ccode;
	  BYTE displayBuffer[255];
	  BYTE workBuffer[12];

	  iportal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       7,
		       5,
		       11,
		       75,
		       3,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	  if (!iportal)
	     return 0;

	  activate_static_portal(iportal);
	  
          NWFSSet(workBuffer, 0, sizeof(workBuffer));

	  sprintf(displayBuffer, "Split Segment Size in MB (up to %uMB): ",
	     (unsigned int)
	       (((LONGLONG)segment_table[index].segment_count *
		 (LONGLONG)IO_BLOCK_SIZE) /
		 (LONGLONG)0x100000));

	  ccode = add_field_to_portal(iportal, 1, 2, WHITE | BGBLUE,
			 displayBuffer, strlen(displayBuffer),
			 workBuffer, sizeof(workBuffer),
			 0, 0, 0, 0, FIELD_ENTRY);
	  if (ccode)
	  {
	     error_portal("****  error:  could not allocate memory ****", 7);
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     return 0;
	  }

	  ccode = input_portal_fields(iportal);
	  if (ccode)
	  {
	     if (iportal)
	     {
		deactivate_static_portal(iportal);
		free_portal(iportal);
	     }
	     return 0;
	  }

	  if (iportal)
	  {
	     deactivate_static_portal(iportal);
	     free_portal(iportal);
	  }

	  SegmentSize = atol(workBuffer);
	  block_count = (ULONG)(((LONGLONG)SegmentSize * (LONGLONG)0x100000) /
					  (LONGLONG)IO_BLOCK_SIZE);
	  if (block_count)
	  {
	     block_count &= 0xFFFFFFF0;
	     if (block_count < segment_table[index].segment_count)
	     {
		for (j=segment_table_count; j > index; j --)
		{
		   nwvp_memmove(&segment_table[j], &segment_table[j - 1], sizeof(segment_info_table));
		   segment_mark_table[j] = segment_mark_table[j - 1];
		}
		segment_table[index+1].segment_count -= block_count;
		segment_table[index].segment_count = block_count;
		segment_table[index+1].segment_offset += block_count;
		segment_mark_table[index+1] = 0;
		segment_table_count++;

		for (i=0; (i < segment_table_count) && (i < 256); i++)
		{
		   LONGLONG size;

		   if (segment_mark_table[i] == 0)
		      sprintf(displayBuffer, "     %2d ", (int) i);
		   else
		      sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
		   write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

		   if (segment_table[i].free_flag == 0)
		   {
		      sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF,
			  &segment_table[i].VolumeName[1],
			  (int) segment_table[i].segment_number);
		      write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

		      if ((int) segment_table[i].total_segments < 10)
			 sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
		      else
			 sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
		      write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
		   }
		   else
		   {
		      sprintf(displayBuffer, "   %4d      *** FREE ***              ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF);
		      write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
		   }

		   size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
		       (LONGLONG)IO_BLOCK_SIZE);

		   if (size < 0x100000)
		      sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		   else
		      sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		   write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

		   size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
			       (LONGLONG)IO_BLOCK_SIZE);
		   if (size < 0x100000)
		      sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
		   else
		      sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
		   write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
		}
		update_static_portal(portal);
		return 0;
	     }
	  }
       }
       else
       {
	  register ULONG i, iportal;
	  register VOLUME *volume;
	  BYTE VolumeName[16];
	  BYTE headerBuffer[255];
	  BYTE displayBuffer[255];

	  for (i=1; (i < 16) && segment_table[index].VolumeName[i]; i++)
	     VolumeName[i - 1] = segment_table[index].VolumeName[i];
	  VolumeName[i - 1] = '\0';

	  volume = GetVolumeHandle(VolumeName);
	  if (!volume)
	     return 0;

	  sprintf(headerBuffer, "Volume %s Information", volume->VolumeName);
	  iportal = make_portal(&ConsoleScreen,
		       headerBuffer,
		       0,
		       7,
		       5,
		       18,
		       75,
		       MAX_SEGMENTS * 20,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       TRUE);

	  if (!iportal)
	     return 0;

	  activate_static_portal(iportal);

	  sprintf(displayBuffer,
		  "[%-15s] sz-%08X (%uMB) block-%dK Fat/Dir-%04X/%04X ",
		  volume->VolumeName,
		  (int)volume->VolumeClusters,
		  (unsigned int)
		   (((LONGLONG)volume->VolumeClusters *
		     (LONGLONG)volume->ClusterSize) /
		      0x100000),
		  (unsigned int)((LONGLONG)volume->ClusterSize / 1024),
		  (unsigned int)volume->FirstFAT,
		  (unsigned int)volume->FirstDirectory);
	  write_portal(iportal, displayBuffer, 0, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer, "NAMESPACES  [ %s%s%s%s%s%s]",
	    (BYTE *)((volume->NameSpaceCount >= 1) ?
	      (BYTE *)get_ns_string((ULONG)volume->NameSpaceID[0]) :
	      (BYTE *)""),
	    (BYTE *)((volume->NameSpaceCount >= 2) ?
	      (BYTE *)get_ns_string((ULONG)volume->NameSpaceID[1]) :
	      (BYTE *)""),
	    (BYTE *)((volume->NameSpaceCount >= 3) ?
	      (BYTE *)get_ns_string((ULONG)volume->NameSpaceID[2]) :
	      (BYTE *)""),
	    (BYTE *)((volume->NameSpaceCount >= 4) ?
	      (BYTE *)get_ns_string((ULONG)volume->NameSpaceID[3]) :
	      (BYTE *)""),
	    (BYTE *)((volume->NameSpaceCount >= 5) ?
	      (BYTE *)get_ns_string((ULONG)volume->NameSpaceID[4]) :
	      (BYTE *)""),
	    (BYTE *)((volume->NameSpaceCount >= 6) ?
	      (BYTE *)get_ns_string((ULONG)volume->NameSpaceID[5]) :
	      (BYTE *)""));
	  write_portal(iportal, displayBuffer, 1, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		  "COMPRESS-%s SUBALLOC-%s MIGRATE-%s AUDIT-%s  (%s)",
		 (volume->VolumeFlags & FILE_COMPRESSION_ON)
		 ? "YES"
		 : "NO ",
		 (volume->VolumeFlags & SUB_ALLOCATION_ON)
		 ? "YES"
		 : "NO ",
		 (volume->VolumeFlags & DATA_MIGRATION_ON)
		 ? "YES"
		 : "NO ",
		 (volume->VolumeFlags & AUDITING_ON)
		 ? "YES"
		 : "NO ",
		  volume->VolumePresent
		  ? "OK"
		  : "NO");
	  write_portal(iportal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

	  for (i=0; (i < volume->NumberOfSegments) && (i < MAX_SEGMENTS); i++)
	  {
	     if (volume->SegmentClusterSize[i])
	     {
		sprintf(displayBuffer,
			"  segment #%d Start-%08X size-%08X (%uMB)",
			(int) i,
			(unsigned int)volume->SegmentClusterStart[i],
			(unsigned int)volume->SegmentClusterSize[i],
			(unsigned int)
			   (((LONGLONG)volume->SegmentClusterSize[i] *
			     (LONGLONG)volume->ClusterSize) /
			      0x100000));
		write_portal(iportal, displayBuffer, 3 + i, 0, BRITEWHITE | BGBLUE);
	     }
	     else
	     {
		sprintf(displayBuffer,
			  "  segment #%d *** error: defined segment missing ***",
			  (int) i);
		write_portal(iportal, displayBuffer, 3 + i, 0, BRITEWHITE | BGBLUE);
	     }
	  }

	  activate_portal(iportal);

	  if (iportal)
	  {
	     deactivate_static_portal(iportal);
	     free_portal(iportal);
	  }
       }
    }
    return 0;
}

ULONG warnFunction(NWSCREEN *screen, ULONG index)
{
    register ULONG mNum, retCode;

    mNum = make_menu(screen,
		     " Exit NWCONFIG ",
		     13,
		     32,
		     2,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     0,
		     0,
		     0,
		     TRUE,
		     0);

    add_item_to_menu(mNum, "Yes", 1);
    add_item_to_menu(mNum, "No", 0);

    retCode = activate_menu(mNum);
    if (retCode == (ULONG) -1)
       retCode = 0;

    free_menu(mNum);

    return retCode;
}


ULONG segmentHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG i, help;
    BYTE displayBuffer[255];

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " Help for Volume Segment Configuration ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       256,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(SegmentHelp) / sizeof (BYTE *); i++)
	     write_portal_cleol(help, SegmentHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F1-Help  F3-Menu  ENTER-View/Split Seg  SPACE-Mark Seg  DEL-Clear Seg");
#else
	  sprintf(displayBuffer, "  F1-Help  ESC-Menu  ENTER-View/Split Seg  SPACE-Mark Seg  DEL-Clear Seg");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       case DEL:
	  build_segment_tables();
	  clear_portal(portal);
	  for (i=0; (i < segment_table_count) && (i < 256); i++)
	  {
	     LONGLONG size;

	     if (segment_mark_table[i] == 0)
		sprintf(displayBuffer, "     %2d ", (int) i);
	     else
		sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
	     write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

	     if (segment_table[i].free_flag == 0)
	     {
		sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF,
			  &segment_table[i].VolumeName[1],
			  (int) segment_table[i].segment_number);
		write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

		if ((int) segment_table[i].total_segments < 10)
		   sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
		else
		   sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
		write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
	     }
	     else
	     {
		sprintf(displayBuffer, "   %4d      *** FREE ***              ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF);
		write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
	     }

	     size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
		       (LONGLONG)IO_BLOCK_SIZE);

	     if (size < 0x100000)
		sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
	     else
		sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
	     write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

	     size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
			       (LONGLONG)IO_BLOCK_SIZE);
	     if (size < 0x100000)
		sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
	     else
		sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
	     write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
	  }
	  update_static_portal(portal);
	  break;

       case SPACE:
	  if (segment_table[index].free_flag != 0)
	  {
	     register ULONG block_count, j;

	     if (segment_mark_table[index] == 0)
	     {
		block_count = 0;
		for (j=index; j > 0; j--)
		{
		   if (segment_table[j - 1].lpart_handle !=
		       segment_table[index].lpart_handle)
			break;

		   if ((segment_table[j - 1].free_flag == 0) ||
		       (segment_mark_table[j - 1] !=0))
			block_count ++;
		}

		for (j=index + 1; j < segment_table_count; j++)
		{
		   if (segment_table[j].lpart_handle !=
		       segment_table[index].lpart_handle)
			break;

		   if ((segment_table[j].free_flag == 0) ||
		       (segment_mark_table[j] !=0))
			block_count ++;
		}

		if (block_count > 7)
		{
		   error_portal("****  Cannot create more than 8 segments/partition ****", 7);
		   break;
		}

		block_count = 0;
		for (j=0; j < segment_table_count; j++)
		{
		   if (segment_mark_table[j] > block_count)
		      block_count = segment_mark_table[j];
		}
		segment_mark_table[index] = block_count + 1;
	     }
	     else
	     {
		for (j=0; j < segment_table_count; j++)
		{
		   if (segment_mark_table[j] > segment_mark_table[index])
			segment_mark_table[j] --;
		}
		segment_mark_table[index] = 0;
	     }

	     for (i=0; (i < segment_table_count) && (i < 256); i++)
	     {
	       LONGLONG size;

	       if (segment_mark_table[i] == 0)
		  sprintf(displayBuffer, "     %2d ", (int) i);
	       else
		  sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
	       write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

	       if (segment_table[i].free_flag == 0)
	       {
		  sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF,
			  &segment_table[i].VolumeName[1],
			  (int) segment_table[i].segment_number);
		  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

		  if ((int) segment_table[i].total_segments < 10)
		     sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
		  else
		     sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
		  write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
	       }
	       else
	       {
		  sprintf(displayBuffer, "   %4d      *** FREE ***              ",
			  (int) segment_table[i].lpart_handle & 0x0000FFFF);
		  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
	       }

	       size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
		       (LONGLONG)IO_BLOCK_SIZE);

	       if (size < 0x100000)
		  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
	       else
		  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
	       write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

	       size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
			       (LONGLONG)IO_BLOCK_SIZE);
	       if (size < 0x100000)
		  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
	       else
		  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
	       write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
	     }
	     update_static_portal(portal);
	     break;
	  }
	  else
	  {
	     error_portal("**** Segment is Already in Use -- Press <Return> ****", 7);
	  }
	  break;

       default:
	  break;
    }
    return 0;
}

ULONG menuKeyboardHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG help, i;
    BYTE displayBuffer[255];

    switch (key)
    {
       case TAB:
	  if (portal)
	  {
#if (LINUX_UTIL)
	     sprintf(displayBuffer, "  F1-Help  F3-Menu  ENTER-View/Split Seg  SPACE-Mark Seg  DEL-Clear Seg");
#else
	     sprintf(displayBuffer, "  F1-Help  ESC-Menu  ENTER-View/Split Seg  SPACE-Mark Seg  DEL-Clear Seg");
#endif
	     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	     get_portal_resp(portal);

#if (LINUX_UTIL)
	     sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Segments");
#else
	     sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Segments");
#endif
	     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  }
	  break;

       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " Help for Volume/Mirroring/Namespace Configuration ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       400,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(GeneralHelp) / sizeof(BYTE *); i++)
	     write_portal_cleol(help, GeneralHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Segments");
#else
	  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Segments");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       default:
	  break;
    }
    return 0;
}

int NWConfig(void)
{
    register ULONG i, retCode;
    BYTE displayBuffer[255];

#if (LINUX_UTIL)
    for (i=0; i < ConsoleScreen.nLines - 1; i++)
       put_char_cleol(&ConsoleScreen, 176 | A_ALTCHARSET, i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
    for (i=0; i < ConsoleScreen.nLines - 1; i++)
       put_char_cleol(&ConsoleScreen, 176, i, CYAN | BGBLUE);
#endif

    sprintf(displayBuffer, CONFIG_NAME);
    put_string_cleol(&ConsoleScreen, displayBuffer, 0, BLUE | BGCYAN);

    sprintf(displayBuffer, COPYRIGHT_NOTICE1);
    put_string_cleol(&ConsoleScreen, displayBuffer, 1, BLUE | BGCYAN);

    sprintf(displayBuffer, COPYRIGHT_NOTICE2);
    put_string_cleol(&ConsoleScreen, displayBuffer, 2, BLUE | BGCYAN);

#if (LINUX_UTIL)
    sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Segments");
#else
    sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Segments");
#endif
    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

    portal = make_portal(&ConsoleScreen,
		       " Mark #   MirrorGrp        Volume       Segment       Offset        Size",
		       0,
		       3,
		       0,
		       13,
		       ConsoleScreen.nCols - 1,
		       256,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       segmentFunction,
		       0,
		       segmentHandler,
		       TRUE);
    if (!portal)
       goto ErrorExit;

    activate_static_portal(portal);

    ScanVolumes();
    for (i=0; i < MAX_DISKS; i++)
    {
       if (SystemDisk[i] && SystemDisk[i]->PhysicalDiskHandle)
          SyncDevice(i);
    }

    build_segment_tables();
    for (i=0; (i < segment_table_count) && (i < 256); i++)
    {
       LONGLONG size;

       if (segment_mark_table[i] == 0)
	  sprintf(displayBuffer, "     %2d ", (int) i);
       else
	  sprintf(displayBuffer, " %2d  %2d ", (int) segment_mark_table[i], (int) i);
       write_portal(portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);

       if (segment_table[i].free_flag == 0)
       {
	  sprintf(displayBuffer, "   %4d    [%15s]   %2d of ",
		  (int) segment_table[i].lpart_handle & 0x0000FFFF,
		  &segment_table[i].VolumeName[1],
		  (int) segment_table[i].segment_number);
	  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);

	  if ((int) segment_table[i].total_segments < 10)
	     sprintf(displayBuffer, "%1d ", (int) segment_table[i].total_segments);
	  else
	     sprintf(displayBuffer, "%2d", (int) segment_table[i].total_segments);
	  write_portal(portal, displayBuffer, i, 45, BRITEWHITE | BGBLUE);
       }
       else
       {
	  sprintf(displayBuffer, "   %4d      *** FREE ***              ",
		  (int) segment_table[i].lpart_handle & 0x0000FFFF);
	  write_portal(portal, displayBuffer, i, 8, BRITEWHITE | BGBLUE);
       }

       size = (LONGLONG)((LONGLONG)segment_table[i].segment_offset *
		       (LONGLONG)IO_BLOCK_SIZE);

       if (size < 0x100000)
	  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(portal, displayBuffer, i, 52, BRITEWHITE | BGBLUE);

       size = (LONGLONG)((LONGLONG)segment_table[i].segment_count *
			       (LONGLONG)IO_BLOCK_SIZE);
       if (size < 0x100000)
	  sprintf(displayBuffer, "  %6u K ", (unsigned)(size / (LONGLONG)1024));
       else
	  sprintf(displayBuffer, "  %6u MB", (unsigned)(size / (LONGLONG)0x100000));
       write_portal(portal, displayBuffer, i, 64, BRITEWHITE | BGBLUE);
    }
    update_static_portal(portal);

    menu = make_menu(&ConsoleScreen,
		     " Available Menu Options ",
		     14,
		     15,
		     5,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     menuFunction,
		     warnFunction,
		     menuKeyboardHandler,
		     TRUE,
		     0);

    if (!menu)
	  goto ErrorExit;

    add_item_to_menu(menu, "Create or Expand Volume (with marked segments)", 1);
    add_item_to_menu(menu, "Delete Netware Volume", 2);
    add_item_to_menu(menu, "Edit Partition Tables", 3);
    add_item_to_menu(menu, "Edit Hotfix and Mirroring", 4);
    add_item_to_menu(menu, "Modify Volume Namespaces/Settings", 5);

    retCode = activate_menu(menu);

ErrorExit:;
    sprintf(displayBuffer, " Exiting ... ");
    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

    if (menu)
       free_menu(menu);

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

#ifndef NWCONFIG_LIB
int main(int argc, char *argv[])
{
    InitNWFS();

    if (InitConsole())
       return 0;

    NWConfig();

    ReleaseConsole();

    return 0;
}
#endif

#endif




