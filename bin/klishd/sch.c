ischeme_t sch = {

 PTYPE_LIST

  PTYPE {
   .name = "ptype1",
   .help = "help1",

   ACTION_LIST

    ACTION {
     .sym = "internal",
     .script = "cat /etc/passwd",
    },

    ACTION {
     .sym = "internal",
     .script = "cat /etc/group",
    },

   END_ACTION_LIST,
  },

  PTYPE {
   .name = "ptype2",
   .help = "help2",
  },

 END_PTYPE_LIST,

 VIEW_LIST

  VIEW {
   .name = "view1",

   COMMAND_LIST

    COMMAND {
     .name = "command1",
     .help = "help1",
    },

    COMMAND {
     .name = "command2",
     .help = "help1",
    },

    COMMAND {
     .name = "command3",
     .help = "help1",
    },

   END_COMMAND_LIST,
  },

  VIEW {
   .name = "view2",
  },

  VIEW {
   .name = "view1",

   COMMAND_LIST

    COMMAND {
     .name = "command4",
     .help = "help1",
    },

    COMMAND {
     .name = "command5",
     .help = "help1",

     PARAM_LIST

      PARAM {
       .name = "param1",
       .help = "helpparam1",
       .ptype = "ptype1",
      },

      PARAM {
       .name = "param2",
       .help = "helpparam2",
       .ptype = "ptype2",

       PARAM_LIST

        PARAM {
         .name = "param3",
         .help = "helpparam1",
         .ptype = "ptype1",
        },

        PARAM {
         .name = "param4",
         .help = "helpparam2",
         .ptype = "ptype2",
        },

       END_PARAM_LIST,
      },

     END_PARAM_LIST,

     ACTION_LIST

      ACTION {
       .sym = "internal",
       .script = "cat /etc/passwd",
      },

      ACTION {
       .sym = "internal",
       .script = "cat /etc/group",
      },

     END_ACTION_LIST,
    },

   END_COMMAND_LIST,
  },

//VIEW {
// },

 END_VIEW_LIST,
};
