NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include/

# Directories
SRCS_DIR = src
OBJS_DIR = obj

# Source files
SRCS_FILES = main.cpp Server.cpp Client.cpp Channel.cpp
SRCS = $(addprefix $(SRCS_DIR)/, $(SRCS_FILES))

# Object files
OBJS = $(patsubst $(SRCS_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS))

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@mkdir -p $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
