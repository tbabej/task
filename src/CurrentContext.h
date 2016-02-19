/*
 * CurrentContext.h
 * Copyright (C) 2016 Ivan Čukić <ivan.cukic(at)kde.org>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef CURRENTCONTEXT_H
#define CURRENTCONTEXT_H

inline std::string getCurrentContext (Context &context)
{
    // If the currently set context was deleted, unset it
    const char * currentContextOverride = getenv ("TASK_CONTEXT");

    return currentContextOverride ? std::string (currentContextOverride)
                                  : context.config.get ("context");
}

#endif /* !CURRENTCONTEXT_H */
