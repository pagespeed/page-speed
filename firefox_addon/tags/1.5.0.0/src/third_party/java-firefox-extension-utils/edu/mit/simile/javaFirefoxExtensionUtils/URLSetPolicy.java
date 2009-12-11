/*
 * Created on Nov 24, 2004
 * Created by dfhuynh
 */
package edu.mit.simile.javaFirefoxExtensionUtils;

import java.net.URL;
import java.security.CodeSource;
import java.security.Permission;
import java.security.PermissionCollection;
import java.security.Permissions;
import java.security.Policy;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;

/**
 * Lets us grant a set of specified permissions to any URL in a set of specified
 * codesources.
 * 
 * @author dfhuynh
 */
public class URLSetPolicy extends Policy {
    static private class MyPermissions extends PermissionCollection {
        private static final long serialVersionUID = 602331721988458546L;
        Permissions m_permissions = new Permissions();

        /* (non-Javadoc)
         * @see java.security.PermissionCollection#add(java.security.Permission)
         */
        public void add(Permission permission) {
            m_permissions.add(permission);
        }

        /* (non-Javadoc)
         * @see java.security.PermissionCollection#implies(java.security.Permission)
         */
        public boolean implies(Permission permission) {
            return m_permissions.implies(permission);
        }

        /* (non-Javadoc)
         * @see java.security.PermissionCollection#elements()
         */
        public Enumeration elements() {
            return m_permissions.elements();
        }
    }
    
    private MyPermissions   m_permissions = new MyPermissions();
    private Policy          m_outerPolicy;
    private Set             m_urls = new HashSet();

    /* (non-Javadoc)
     * @see java.security.Policy#refresh()
     */
    public void refresh() {
        if (m_outerPolicy != null) {
            m_outerPolicy.refresh();
        }
    }
    
    /* (non-Javadoc)
     * @see java.security.Policy#getPermissions(java.security.CodeSource)
     */
    public PermissionCollection getPermissions(CodeSource codesource) {
        PermissionCollection pc = m_outerPolicy != null ?
                m_outerPolicy.getPermissions(codesource) :
                new Permissions();
        
        URL url = codesource.getLocation();
        if (url != null) {
            String s = url.toExternalForm();
            if (m_urls.contains(s) || "file:".equals(s)) {
                Enumeration e = m_permissions.elements();
                while (e.hasMoreElements()) {
                    pc.add((Permission) e.nextElement());
                }
            }
        }
        
        return pc;
    }
    
    /**
     * Sets the outer policy so that we can defer to it for code sources that
     * we are not told about.
     * 
     * @param policy
     */
    public void setOuterPolicy(Policy policy) {
        m_outerPolicy = policy;
    }
    
    public void addPermission(Permission permission) {
        m_permissions.add(permission);
    }
    
    public void addURL(URL url) {
        m_urls.add(url.toExternalForm());
    }
}
