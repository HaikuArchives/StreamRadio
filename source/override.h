/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _OVERRIDE_H
#define _OVERRIDE_H

/*
 * The override mark is especially helpful when using an unstable API, like
 * BPrivate::Network, but we may not have it in all the compilers we use.
 */
#if __cplusplus < 201103L
#define override
#endif

#endif // _OVERRIDE_H
