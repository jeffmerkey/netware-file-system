
/***************************************************************************
*
*   Copyright (c) 1997-2001 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the Lesser GNU Public License as published by the
*   Free Software Foundation, version 2.1, or any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
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
*   FILE     :  IMGHELP.H
*   DESCRIP  :  NWFS Imaging Help Text
*   DATE     :  December 21, 2022
*
*
***************************************************************************/

BYTE *AllHelp[]=
{
   " ",
   " This utility allows you to make image sets of an entire NetWare",
   " Server's volumes, files, and user data.  This utility provides an",
   " extremely simple method for system administrators to store volumes",
   " and their associated data off-line and to be able to restore them",
   " quickly and easily to any server in a Network.  This utility provides",
   " NetWare volume cloning capabilities for those who need to replicate",
   " and clone server configurations across several machines.",
   " ",
   " This utility creates 'Volume Sets'.  A 'Volume Set' is a compressed",
   " archive of from 1 to 256 separate NetWare volumes that are managed",
   " as a collection of volume image files.  These image files can be",
   " stored remotely on a server, and downloaded via MS-DOS, Linux, or",
   " Windows NT/2000 versions of this utility.",
   " ",
   " There are various help panels within each section of this utility",
   " that will guide you through the process of creating, managing, and",
   " restoring volume image sets." ,
   " ",
   " To image a server configuration, select the Create Image option from",
   " the main menu.  To Restore a Volume, select the Restore Image Set",
   " option from the main menu.  The Change Image Setting selection",
   " will allow you to change imaging options.  For example, NetWare",
   " volumes also contain deleted files.  There is an option to allow",
   " these deleted files to be imaged and restored as well as normal",
   " user data.  There is also an option for automatic disk partitioning",
   " during volume restore, and you can set the image set default name",
   " from this panel.",
   " ",
   " This utility maintains a log file that records events that occur",
   " during volume image and restore operations.  By default, this file is",
   " named NWFS.LOG, however, the name of the log file for a given session",
   " is user configurable.",
   " ",
   " Novell, Inc. and NetWare are registered Trademarks of Novell, Inc.",
   " Windows NT/2000 is a registered trademark of Microsoft Corporation.",
   " Linux is a registered trademark of Linus Torvalds.",
   " ",
};

BYTE *ImageHelp[]=
{
   " ",
   " To create a NetWare Volume Image Set, you press the TAB key",
   " and mark those volumes you want to include in this image set.",
   " After you have marked the volumes you want in this set, then",
   " you select the Image Marked Volumes option on the main menu.",
   " this will start the imaging process.",
   " ",
   " This utility will maintain a log file for any errors that may",
   " occur during volume mounting and imaging.  It is highly advised",
   " that you run VREPAIR from NetWare (or an equivalent program) prior",
   " to imaging any volumes to make certain the volumes are clean and",
   " free of errors.  By default, this log file is called NWFS.LOG",
   " ",
   " The name of the image set is displayed in te upper left corner of",
   " the image portal.  There are also menu options to delete volume sets",
   " you no longer need, and to change the name of an image set if you",
   " don't like the current name.",
   " ",
};

BYTE *RestoreHelp[]=
{
   " ",
   " To Restore a volume image set, you use the Select Image Set option",
   " and select an image set from the popup menu that appears.  A listing",
   " of volumes that are contained within this image set will be displayed",
   " in the Restore portal after you have selected the image set you wish",
   " to use for your restore operation.",
   " ",
   " You can select which volumes from this set will be restored, and you",
   " can cross restore one volume's image set data to an entirely different",
   " volume name on the target system if you desire.  You can also auto",
   " create volumes and disk partitions if they do not exist on the target",
   " system.",
   " ",
   " Press the TAB key to select the restore portal if you want to change",
   " any volume restore settings.  There is a help panel for the restore",
   " portal that describes more fully the editing options for a particular",
   " volume image set.",
   " ",
   " There is also an option for deleting volume sets you may no longer",
   " need.",
   " ",
};

BYTE *ImageSettingsHelp[]=
{
   " ",
   " To select an individual item from this portal, position the cursor",
   " bar over the item you wish to edit.",
   " ",
   " Press the SPACE bar to select individual volumes that you wish to",
   " include within a particular image set.  Selected volumes will have",
   " the *** characters beside each volume name indicating that this",
   " particular volume has been selected for inclusion in the volume set.",
   " ",
   " After you have selected the volumes you want, press the ESC/F3",
   " key to return to the menu, and select the image option.  If you",
   " have already created several image sets within your current",
   " directory, the utility will detect this and attempt to assign",
   " a different name for you and prompt you before proceeding.  You ",
   " do have the option of overwriting your existing volume set.",
   " ",
   " There are also menu options for deleting image sets you no longer",
   " need or want to discard.  If for any reason you want to assign your",
   " own name to an image set, then use the menu option to set the image",
   " name to your selection.",
   " ",
};

BYTE *RestoreSettingsHelp[]=
{
   " ",
   " To select an individual item from this portal, position the cursor",
   " bar over the item you wish to edit.",
   " ",
   " Press the SPACE bar to select volumes you want this utility to",
   " auto-create.  If the utility does not detect a volume with this",
   " name on the target system, it will by default attempt to auto-create",
   " the volume, then restore it to the system.",
   " ",
   " You can also cross assign volumes from within volume sets to volumes",
   " with a different name.  For example, you may have imaged a volume named",
   " SYS, and you want to restore the data to a volume with a different",
   " name.  The utility allows you to do this.  If you press the ENTER",
   " key on a volume you have positioned the cursor bar on top of, a",
   " popup menu will allow you to edit the volume configuration itself.",
   " This panel allows you to change the volume size, cluster size, feature",
   " flags for the volume, and you can also rename the volume or assign",
   " the volume image to a pre-existing volume on the machine (which will",
   " be overwritten by the selected volume image during restore).",
   " ",
   " If the utility detects a volume with a matching name, it will by",
   " default try to assign the image set volume data to this volume.",
   " You can, however, change any of these assignments.  You cannot ",
   " auto-create a volume that already exists because NetWare does not",
   " allow two volumes with the non-unique names on the same physical",
   " single server.",
   " ",
   " The INS key allows you to select a target volume from a popup menu",
   " of pre-existing NetWare volumes.  The DEL key will remove a target",
   " volume assignment from a specific volume image.",
   " "
   " This utility also allows you to configure any of the volumes on the",
   " system if for some reason you want to adjust mirroring, hotfixing,",
   " namespace configuration, volume striping, or volumes names.",
   " You should invoke the volume manager from the main menu if you want",
   " to manually reconfigure your volume setup.",
   " ",
   " This utility also has an automatic mode, and will automatically",
   " partition the disks and create NetWare volumes that are the closest",
   " match to the original configurations contained in your volume image",
   " set.",
   " ",
};

