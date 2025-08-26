# ************************************************************************ #
#                			        NAME                                   #
# ************************************************************************ #

NAME = webserv

# ************************************************************************ #
#                			     DIRECTORIES                               #
# ************************************************************************ #

INCLUDES	= includes -I./srcs/response -I./srcs/request
SRCS_DIR	= srcs
OBJS_DIR	= obj

# ************************************************************************ #
#                			 SRCS && OBJECTS FILES                         #
# ************************************************************************ #

WEBSERV		= $(addprefix $(SRCS_DIR)/, server.cpp\
										response/response.cpp\
										request/request.cpp\
										utils/utils.cpp)
SRCS		= ${WEBSERV}
CPP_OBJS	= ${WEBSERV:${SRCS_DIR}/%.cpp=$(OBJS_DIR)/%.o}
OBJS		= ${CPP_OBJS}


# ************************************************************************ #
#                    			 COMPILATION                               #
# ************************************************************************ #

CC			= c++
CFLAGS		= -Wall -Werror -Wextra -std=c++98 -I$(INCLUDES) -I./response -I./request
RM			= rm -rf

# ************************************************************************ #
#                  		       	 PROCESS                                   #
# ************************************************************************ #

all:    $(NAME)

$(NAME): includes/colors.hpp $(OBJS_DIR) $(OBJS)
	@echo "\033[1;33m  COMPILING WEBSERV \n"
	@$(CC) $(OBJS) $(CFLAGS) -o $(NAME)
	@echo "\033[1;32m ./$(NAME) created\n"

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp includes/colors.hpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

# ************************************************************************ #
#                  		     CLEANUP SESSION                               #
# ************************************************************************ #

clean:
	@$(RM) $(OBJS_DIR)
	@$(RM) $(OBJS:.o=.d)
	@echo "\033[1;32m🫧  DONE 🫧"

fclean: clean
	@$(RM) $(NAME) includes/colors.hpp
	@echo "\033[1;32m🫧  FCLEAN DONE 🫧"

re: fclean all
	@echo "\033[1;32m🫧  RE DONE 🫧"

.PHONY: all clean fclean re


# ************************************************************************ #
#                   	       DEPENDENCIES                                #
# ************************************************************************ #

-include $(OBJS:.o=.d)

# ************************************************************************ #
#                   	          COLORS                                   #
# ************************************************************************ #

includes/colors.hpp:
	@echo '#ifndef COLORS_HPP' > $@
	@echo '#define COLORS_HPP' >> $@
	@echo '#define RESET		"\033[0m"' >> $@
	@echo '#define WHITE		"\033[1;97m"' >> $@
	@echo '#define FUCHSIA		"\033[1;38;5;201m"' >> $@
	@echo '#define BLUE_BRIGHT	"\033[1;94m"' >> $@
	@echo '#define GREEN		"\033[1;32m"' >> $@
	@echo '#define MAGENTA		"\033[1;35m"' >> $@
	@echo '#define CYAN			"\033[1;36m"' >> $@
	@echo '#define RED_BOLD		"\033[1;91m"' >> $@
	@echo '#define RED			"\033[0;91m"' >> $@
	@echo '#define YELLOW		"\033[1;93m"' >> $@
	@echo '#define ORANGE		"\033[1;38;5;208m"' >> $@
	@echo '#define PINK			"\033[1;38;5;205m"' >> $@
	@echo '#define TURQUOISE	"\033[1;38;5;45m"' >> $@
	@echo '#define GOLD			"\033[1;38;5;220m"' >> $@
	@echo '#define CORAL		"\033[1;38;5;203m"' >> $@
	@echo '#endif' >> $@